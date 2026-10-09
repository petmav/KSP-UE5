using System.CodeDom;

namespace Microsoft.CSharp;

internal sealed class CSharpMemberAttributeConverter : CSharpModifierAttributeConverter
{
	public static CSharpMemberAttributeConverter Default { get; } = new CSharpMemberAttributeConverter();


	protected override string[] Names { get; } = new string[5] { "Public", "Protected", "Protected Internal", "Internal", "Private" };


	protected override object[] Values { get; } = new object[5]
	{
		MemberAttributes.Public,
		MemberAttributes.Family,
		MemberAttributes.FamilyOrAssembly,
		MemberAttributes.Assembly,
		MemberAttributes.Private
	};


	protected override object DefaultValue => MemberAttributes.Private;

	private CSharpMemberAttributeConverter()
	{
	}
}
