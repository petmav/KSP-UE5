namespace System.Configuration;

internal enum UserConfigLocationOption : uint
{
	Product = 32u,
	Product_VersionMajor = 33u,
	Product_VersionMinor = 34u,
	Product_VersionBuild = 36u,
	Product_VersionRevision = 40u,
	Company_Product = 48u,
	Company_Product_VersionMajor = 49u,
	Company_Product_VersionMinor = 50u,
	Company_Product_VersionBuild = 52u,
	Company_Product_VersionRevision = 56u,
	Evidence = 64u,
	Other = 32768u
}
