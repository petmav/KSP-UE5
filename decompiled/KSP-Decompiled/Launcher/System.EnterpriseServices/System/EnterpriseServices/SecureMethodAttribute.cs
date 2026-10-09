using System.Runtime.InteropServices;

namespace System.EnterpriseServices;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Method, Inherited = true, AllowMultiple = false)]
[ComVisible(false)]
public sealed class SecureMethodAttribute : Attribute
{
}
