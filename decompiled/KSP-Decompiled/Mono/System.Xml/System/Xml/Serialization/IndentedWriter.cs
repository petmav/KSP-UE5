using System.IO;

namespace System.Xml.Serialization;

internal class IndentedWriter
{
	private TextWriter writer;

	private bool needIndent;

	private int indentLevel;

	private bool compact;

	internal int Indent
	{
		get
		{
			return indentLevel;
		}
		set
		{
			indentLevel = value;
		}
	}

	internal IndentedWriter(TextWriter writer, bool compact)
	{
		this.writer = writer;
		this.compact = compact;
	}

	internal void Write(string s)
	{
		if (needIndent)
		{
			WriteIndent();
		}
		writer.Write(s);
	}

	internal void Write(char c)
	{
		if (needIndent)
		{
			WriteIndent();
		}
		writer.Write(c);
	}

	internal void WriteLine(string s)
	{
		if (needIndent)
		{
			WriteIndent();
		}
		writer.WriteLine(s);
		needIndent = true;
	}

	internal void WriteLine()
	{
		writer.WriteLine();
		needIndent = true;
	}

	internal void WriteIndent()
	{
		needIndent = false;
		if (!compact)
		{
			for (int i = 0; i < indentLevel; i++)
			{
				writer.Write("    ");
			}
		}
	}
}
