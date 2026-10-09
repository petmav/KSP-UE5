using System.CodeDom;
using System.CodeDom.Compiler;
using System.Security.Permissions;
using Unity;

namespace System.Data;

[Obsolete("TypedDataSetGenerator class will be removed in a future release. Please use System.Data.Design.TypedDataSetGenerator in System.Design.dll.")]
[HostProtection(SecurityAction.LinkDemand, SharedState = true, Synchronization = true)]
public class TypedDataSetGenerator
{
	public TypedDataSetGenerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static void Generate(DataSet dataSet, CodeNamespace codeNamespace, ICodeGenerator codeGen)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static string GenerateIdName(string name, ICodeGenerator codeGen)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
