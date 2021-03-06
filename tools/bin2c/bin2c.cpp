/*
 * Copyright 2011-2019 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include <bx/allocator.h>
#include <bx/commandline.h>
#include <bx/file.h>
#include <bx/string.h>

class Bin2cWriter : public bx::WriterI
{
public:
	Bin2cWriter(bx::AllocatorI* _allocator, const bx::StringView& _name)
		: m_mb(_allocator)
		, m_mw(&m_mb)
		, m_name(_name)
	{
	}

	virtual ~Bin2cWriter()
	{
	}

	virtual int32_t write(const void* _data, int32_t _size, bx::Error* _err) override
	{
		return bx::write(&m_mw, _data, _size, _err);
	}

	void output(bx::WriterI* m_writer)
	{
#define HEX_DUMP_WIDTH 16
#define HEX_DUMP_SPACE_WIDTH 96
#define HEX_DUMP_FORMAT "%-" BX_STRINGIZE(HEX_DUMP_SPACE_WIDTH) "." BX_STRINGIZE(HEX_DUMP_SPACE_WIDTH) "s"
		const char* data = (const char*)m_mb.more(0);
		uint32_t size = uint32_t(bx::seek(&m_mw) );

		bx::Error err;

		bx::write(
			  m_writer
			, &err
			, "static const uint8_t %.*s[%d] =\n{\n"
			, m_name.getLength()
			, m_name.getPtr()
			, size
			);

		if (NULL != data)
		{
			char hex[HEX_DUMP_SPACE_WIDTH+1];
			char ascii[HEX_DUMP_WIDTH+1];
			uint32_t hexPos = 0;
			uint32_t asciiPos = 0;
			for (uint32_t ii = 0; ii < size; ++ii)
			{
				bx::snprintf(&hex[hexPos], sizeof(hex)-hexPos, "0x%02x, ", data[asciiPos]);
				hexPos += 6;

				ascii[asciiPos] = bx::isPrint(data[asciiPos]) && data[asciiPos] != '\\' ? data[asciiPos] : '.';
				asciiPos++;

				if (HEX_DUMP_WIDTH == asciiPos)
				{
					ascii[asciiPos] = '\0';
					bx::write(m_writer, &err, "\t" HEX_DUMP_FORMAT "// %s\n", hex, ascii);
					data += asciiPos;
					hexPos   = 0;
					asciiPos = 0;
				}
			}

			if (0 != asciiPos)
			{
				ascii[asciiPos] = '\0';
				bx::write(m_writer, &err, "\t" HEX_DUMP_FORMAT "// %s\n", hex, ascii);
			}
		}

		bx::write(m_writer, &err, "};\n");
#undef HEX_DUMP_WIDTH
#undef HEX_DUMP_SPACE_WIDTH
#undef HEX_DUMP_FORMAT
	}

	bx::MemoryBlock  m_mb;
	bx::MemoryWriter m_mw;
	bx::StringView   m_name;
};

void help(const char* _error = NULL)
{
	bx::WriterI* stdOut = bx::getStdOut();
	bx::Error err;

	if (NULL != _error)
	{
		bx::write(stdOut, &err, "Error:\n%s\n\n", _error);
	}

	bx::write(stdOut, &err
		, "bin2c, binary to C\n"
		  "Copyright 2011-2019 Branimir Karadzic. All rights reserved.\n"
		  "License: https://github.com/bkaradzic/bx#license-bsd-2-clause\n\n"
		);

	bx::write(stdOut, &err
		, "Usage: bin2c -f <in> -o <out> -n <name>\n"
		  "\n"
		  "Options:\n"
		  "  -f <file path>    Input file path.\n"
		  "  -o <file path>    Output file path.\n"
		  "  -n <name>         Array name.\n"
		  "\n"
		  "For additional information, see https://github.com/bkaradzic/bx\n"
		);
}

int main(int _argc, const char* _argv[])
{
	bx::CommandLine cmdLine(_argc, _argv);

	if (cmdLine.hasArg('h', "help") )
	{
		help();
		return bx::kExitFailure;
	}

	bx::FilePath filePath = cmdLine.findOption('f');
	if (filePath.isEmpty() )
	{
		help("Input file name must be specified.");
		return bx::kExitFailure;
	}

	bx::FilePath outFilePath = cmdLine.findOption('o');
	if (outFilePath.isEmpty() )
	{
		help("Output file name must be specified.");
		return bx::kExitFailure;
	}

	bx::StringView name = cmdLine.findOption('n');
	if (name.isEmpty() )
	{
		name.set("data");
	}

	void* data = NULL;
	uint32_t size = 0;

	bx::FileReader fr;
	if (bx::open(&fr, filePath) )
	{
		size = uint32_t(bx::getSize(&fr) );

		bx::DefaultAllocator allocator;
		data = BX_ALLOC(&allocator, size);
		bx::read(&fr, data, size);
		bx::close(&fr);

		bx::FileWriter fw;
		if (bx::open(&fw, outFilePath) )
		{
			Bin2cWriter writer(&allocator, name);
			bx::write(&writer, data, size);

			writer.output(&fw);
			bx::close(&fw);
		}

		BX_FREE(&allocator, data);
	}

	return 0;
}
