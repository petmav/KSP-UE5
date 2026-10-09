using System;
using UnityEngine.Bindings;

namespace UnityEngine;

[VisibleToOtherModules]
[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct, Inherited = false)]
internal sealed class NativeClassAttribute : Attribute
{
	public string QualifiedNativeName { get; private set; }

	public string Declaration { get; private set; }

	public NativeClassAttribute(string qualifiedCppName)
	{
		QualifiedNativeName = qualifiedCppName;
		Declaration = "class " + qualifiedCppName;
	}

	public NativeClassAttribute(string qualifiedCppName, string declaration)
	{
		QualifiedNativeName = qualifiedCppName;
		Declaration = declaration;
	}
}
