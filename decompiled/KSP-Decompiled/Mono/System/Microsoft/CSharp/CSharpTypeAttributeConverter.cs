using System.Reflection;

namespace Microsoft.CSharp;

internal sealed class CSharpTypeAttributeConverter : CSharpModifierAttributeConverter
{
	public static CSharpTypeAttributeConverter Default { get; } = new CSharpTypeAttributeConverter();


	protected override string[] Names { get; } = new string[2] { "Public", "Internal" };


	protected override object[] Values { get; } = new object[2]
	{
		TypeAttributes.Public,
		TypeAttributes.NotPublic
	};


	protected override object DefaultValue => TypeAttributes.NotPublic;

	private CSharpTypeAttributeConverter()
	{
	}
}
