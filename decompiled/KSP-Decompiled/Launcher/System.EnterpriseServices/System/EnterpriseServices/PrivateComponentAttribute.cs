using System.Runtime.InteropServices;

namespace System.EnterpriseServices;

[AttributeUsage(AttributeTargets.Class, Inherited = true)]
[ComVisible(false)]
public sealed class PrivateComponentAttribute : Attribute
{
}
