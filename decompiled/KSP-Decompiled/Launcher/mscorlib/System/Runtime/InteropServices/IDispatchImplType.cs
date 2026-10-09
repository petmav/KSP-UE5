namespace System.Runtime.InteropServices;

[Serializable]
[ComVisible(true)]
[Obsolete("The IDispatchImplAttribute is deprecated.", false)]
public enum IDispatchImplType
{
	SystemDefinedImpl,
	InternalImpl,
	CompatibleImpl
}
