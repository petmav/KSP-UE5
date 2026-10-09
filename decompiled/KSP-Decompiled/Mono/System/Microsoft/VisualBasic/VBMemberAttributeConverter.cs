using System.CodeDom;

namespace Microsoft.VisualBasic;

internal sealed class VBMemberAttributeConverter : VBModifierAttributeConverter
{
	public static VBMemberAttributeConverter Default { get; } = new VBMemberAttributeConverter();


	protected override string[] Names { get; } = new string[5] { "Public", "Protected", "Protected Friend", "Friend", "Private" };


	protected override object[] Values { get; } = new object[5]
	{
		MemberAttributes.Public,
		MemberAttributes.Family,
		MemberAttributes.FamilyOrAssembly,
		MemberAttributes.Assembly,
		MemberAttributes.Private
	};


	protected override object DefaultValue => MemberAttributes.Private;

	private VBMemberAttributeConverter()
	{
	}
}
