using System;

namespace JetBrains.Annotations;

[BaseTypeRequired(typeof(Attribute))]
[AttributeUsage(AttributeTargets.Class, AllowMultiple = true)]
public sealed class BaseTypeRequiredAttribute : Attribute
{
	[NotNull]
	public Type BaseType { get; }

	public BaseTypeRequiredAttribute([NotNull] Type baseType)
	{
		BaseType = baseType;
	}
}
