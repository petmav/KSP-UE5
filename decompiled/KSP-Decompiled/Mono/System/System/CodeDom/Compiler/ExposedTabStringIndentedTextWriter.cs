using System.IO;

namespace System.CodeDom.Compiler;

internal sealed class ExposedTabStringIndentedTextWriter : IndentedTextWriter
{
	internal string TabString { get; }

	public ExposedTabStringIndentedTextWriter(TextWriter writer, string tabString)
		: base(writer, tabString)
	{
		TabString = tabString ?? "    ";
	}

	internal void InternalOutputTabs()
	{
		TextWriter innerWriter = base.InnerWriter;
		for (int i = 0; i < base.Indent; i++)
		{
			innerWriter.Write(TabString);
		}
	}
}
