namespace System.Configuration;

[Flags]
internal enum ConfigurationLockType
{
	Attribute = 1,
	Element = 2,
	Exclude = 0x10
}
