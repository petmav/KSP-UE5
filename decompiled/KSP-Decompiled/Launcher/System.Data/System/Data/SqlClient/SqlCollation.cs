using System.Data.SqlTypes;

namespace System.Data.SqlClient;

internal sealed class SqlCollation
{
	private const uint IgnoreCase = 1048576u;

	private const uint IgnoreNonSpace = 2097152u;

	private const uint IgnoreWidth = 4194304u;

	private const uint IgnoreKanaType = 8388608u;

	private const uint BinarySort = 16777216u;

	internal const uint MaskLcid = 1048575u;

	private const int LcidVersionBitOffset = 28;

	private const uint MaskLcidVersion = 4026531840u;

	private const uint MaskCompareOpt = 32505856u;

	internal uint info;

	internal byte sortId;

	internal int LCID
	{
		get
		{
			return (int)(info & 0xFFFFF);
		}
		set
		{
			int num = value & 0xFFFFF;
			int num2 = FirstSupportedCollationVersion(num) << 28;
			info = (info & 0x1F00000u) | (uint)num | (uint)num2;
		}
	}

	internal SqlCompareOptions SqlCompareOptions
	{
		get
		{
			SqlCompareOptions sqlCompareOptions = SqlCompareOptions.None;
			if ((info & 0x100000u) != 0)
			{
				sqlCompareOptions |= SqlCompareOptions.IgnoreCase;
			}
			if ((info & 0x200000u) != 0)
			{
				sqlCompareOptions |= SqlCompareOptions.IgnoreNonSpace;
			}
			if ((info & 0x400000u) != 0)
			{
				sqlCompareOptions |= SqlCompareOptions.IgnoreWidth;
			}
			if ((info & 0x800000u) != 0)
			{
				sqlCompareOptions |= SqlCompareOptions.IgnoreKanaType;
			}
			if ((info & 0x1000000u) != 0)
			{
				sqlCompareOptions |= SqlCompareOptions.BinarySort;
			}
			return sqlCompareOptions;
		}
		set
		{
			uint num = 0u;
			if ((value & SqlCompareOptions.IgnoreCase) != 0)
			{
				num |= 0x100000u;
			}
			if ((value & SqlCompareOptions.IgnoreNonSpace) != 0)
			{
				num |= 0x200000u;
			}
			if ((value & SqlCompareOptions.IgnoreWidth) != 0)
			{
				num |= 0x400000u;
			}
			if ((value & SqlCompareOptions.IgnoreKanaType) != 0)
			{
				num |= 0x800000u;
			}
			if ((value & SqlCompareOptions.BinarySort) != 0)
			{
				num |= 0x1000000u;
			}
			info = (info & 0xFFFFFu) | num;
		}
	}

	private static int FirstSupportedCollationVersion(int lcid)
	{
		return lcid switch
		{
			1044 => 2, 
			1047 => 2, 
			1056 => 2, 
			1065 => 2, 
			1068 => 2, 
			1070 => 2, 
			1071 => 1, 
			1081 => 1, 
			1082 => 2, 
			1083 => 2, 
			1087 => 1, 
			1090 => 2, 
			1091 => 1, 
			1092 => 1, 
			1093 => 2, 
			1101 => 2, 
			1105 => 2, 
			1106 => 2, 
			1107 => 2, 
			1108 => 2, 
			1114 => 1, 
			1121 => 2, 
			1122 => 2, 
			1123 => 2, 
			1125 => 1, 
			1133 => 2, 
			1146 => 2, 
			1148 => 2, 
			1150 => 2, 
			1152 => 2, 
			1153 => 2, 
			1155 => 2, 
			1157 => 2, 
			1164 => 2, 
			2074 => 2, 
			2092 => 2, 
			2107 => 2, 
			2143 => 2, 
			3076 => 1, 
			3098 => 2, 
			5124 => 2, 
			5146 => 2, 
			8218 => 2, 
			_ => 0, 
		};
	}

	internal static bool AreSame(SqlCollation a, SqlCollation b)
	{
		if (a == null || b == null)
		{
			return a == b;
		}
		if (a.info == b.info)
		{
			return a.sortId == b.sortId;
		}
		return false;
	}
}
