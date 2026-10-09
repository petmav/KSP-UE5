using System.Reflection;

namespace Microsoft.VisualBasic;

internal sealed class VBTypeAttributeConverter : VBModifierAttributeConverter
{
	public static VBTypeAttributeConverter Default { get; } = new VBTypeAttributeConverter();


	protected override string[] Names { get; } = new string[2] { "Public", "Friend" };


	protected override object[] Values { get; } = new object[2]
	{
		TypeAttributes.Public,
		TypeAttributes.NotPublic
	};


	protected override object DefaultValue => TypeAttributes.Public;

	private VBTypeAttributeConverter()
	{
	}
}
