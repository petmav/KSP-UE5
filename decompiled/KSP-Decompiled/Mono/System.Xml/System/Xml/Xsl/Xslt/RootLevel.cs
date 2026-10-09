namespace System.Xml.Xsl.Xslt;

internal class RootLevel : StylesheetLevel
{
	public RootLevel(Stylesheet principal)
	{
		Imports = new Stylesheet[1] { principal };
	}
}
