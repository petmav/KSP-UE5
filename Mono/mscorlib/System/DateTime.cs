using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;

namespace System;

[Serializable]
[StructLayout(LayoutKind.Auto)]
public struct DateTime : IComparable, IFormattable, IConvertible, ISerializable, IComparable<DateTime>, IEquatable<DateTime>
{
	private const long TicksPerMillisecond = 10000L;

	private const long TicksPerSecond = 10000000L;

	private const long TicksPerMinute = 600000000L;

	private const long TicksPerHour = 36000000000L;

	private const long TicksPerDay = 864000000000L;

	private const int MillisPerSecond = 1000;

	private const int MillisPerMinute = 60000;

	private const int MillisPerHour = 3600000;

	private const int MillisPerDay = 86400000;

	private const int DaysPerYear = 365;

	private const int DaysPer4Years = 1461;

	private const int DaysPer100Years = 36524;

	private const int DaysPer400Years = 146097;

	private const int DaysTo1601 = 584388;

	private const int DaysTo1899 = 693593;

	internal const int DaysTo1970 = 719162;

	private const int DaysTo10000 = 3652059;

	internal const long MinTicks = 0L;

	internal const long MaxTicks = 3155378975999999999L;

	private const long MaxMillis = 315537897600000L;

	private const long FileTimeOffset = 504911232000000000L;

	private const long DoubleDateOffset = 599264352000000000L;

	private const long OADateMinAsTicks = 31241376000000000L;

	private const double OADateMinAsDouble = -657435.0;

	private const double OADateMaxAsDouble = 2958466.0;

	private const int DatePartYear = 0;

	private const int DatePartDayOfYear = 1;

	private const int DatePartMonth = 2;

	private const int DatePartDay = 3;

	private static readonly int[] DaysToMonth365 = new int[13]
	{
		0, 31, 59, 90, 120, 151, 181, 212, 243, 273,
		304, 334, 365
	};

	private static readonly int[] DaysToMonth366 = new int[13]
	{
		0, 31, 60, 91, 121, 152, 182, 213, 244, 274,
		305, 335, 366
	};

	public static readonly DateTime MinValue = new DateTime(0L, DateTimeKind.Unspecified);

	public static readonly DateTime MaxValue = new DateTime(3155378975999999999L, DateTimeKind.Unspecified);

	private const ulong TicksMask = 4611686018427387903uL;

	private const ulong FlagsMask = 13835058055282163712uL;

	private const ulong LocalMask = 9223372036854775808uL;

	private const long TicksCeiling = 4611686018427387904L;

	private const ulong KindUnspecified = 0uL;

	private const ulong KindUtc = 4611686018427387904uL;

	private const ulong KindLocal = 9223372036854775808uL;

	private const ulong KindLocalAmbiguousDst = 13835058055282163712uL;

	private const int KindShift = 62;

	private const string TicksField = "ticks";

	private const string DateDataField = "dateData";

	private ulong dateData;

	internal long InternalTicks => (long)(dateData & 0x3FFFFFFFFFFFFFFFL);

	private ulong InternalKind => dateData & 0xC000000000000000uL;

	public DateTime Date
	{
		get
		{
			long internalTicks = InternalTicks;
			return new DateTime((ulong)(internalTicks - internalTicks % 864000000000L) | InternalKind);
		}
	}

	public int Day => GetDatePart(3);

	public DayOfWeek DayOfWeek => (DayOfWeek)((InternalTicks / 864000000000L + 1) % 7);

	public int DayOfYear => GetDatePart(1);

	public int Hour => (int)(InternalTicks / 36000000000L % 24);

	public DateTimeKind Kind => InternalKind switch
	{
		0uL => DateTimeKind.Unspecified, 
		4611686018427387904uL => DateTimeKind.Utc, 
		_ => DateTimeKind.Local, 
	};

	public int Millisecond => (int)(InternalTicks / 10000 % 1000);

	public int Minute => (int)(InternalTicks / 600000000 % 60);

	public int Month => GetDatePart(2);

	public static DateTime Now
	{
		get
		{
			DateTime utcNow = UtcNow;
			bool isAmbiguousLocalDst = false;
			long ticks = TimeZoneInfo.GetDateTimeNowUtcOffsetFromUtc(utcNow, out isAmbiguousLocalDst).Ticks;
			long num = utcNow.Ticks + ticks;
			if (num > 3155378975999999999L)
			{
				return new DateTime(3155378975999999999L, DateTimeKind.Local);
			}
			if (num < 0)
			{
				return new DateTime(0L, DateTimeKind.Local);
			}
			return new DateTime(num, DateTimeKind.Local, isAmbiguousLocalDst);
		}
	}

	public static DateTime UtcNow
	{
		[SecuritySafeCritical]
		get
		{
			return new DateTime((ulong)(GetSystemTimeAsFileTime() + 504911232000000000L) | 0x4000000000000000uL);
		}
	}

	public int Second => (int)(InternalTicks / 10000000 % 60);

	public long Ticks => InternalTicks;

	public TimeSpan TimeOfDay => new TimeSpan(InternalTicks % 864000000000L);

	public static DateTime Today => Now.Date;

	public int Year => GetDatePart(0);

	public DateTime(long ticks)
	{
		if (ticks < 0 || ticks > 3155378975999999999L)
		{
			throw new ArgumentOutOfRangeException("ticks", Environment.GetResourceString("Ticks must be between DateTime.MinValue.Ticks and DateTime.MaxValue.Ticks."));
		}
		dateData = (ulong)ticks;
	}

	private DateTime(ulong dateData)
	{
		this.dateData = dateData;
	}

	public DateTime(long ticks, DateTimeKind kind)
	{
		if (ticks < 0 || ticks > 3155378975999999999L)
		{
			throw new ArgumentOutOfRangeException("ticks", Environment.GetResourceString("Ticks must be between DateTime.MinValue.Ticks and DateTime.MaxValue.Ticks."));
		}
		if (kind < DateTimeKind.Unspecified || kind > DateTimeKind.Local)
		{
			throw new ArgumentException(Environment.GetResourceString("Invalid DateTimeKind value."), "kind");
		}
		dateData = (ulong)(ticks | ((long)kind << 62));
	}

	internal DateTime(long ticks, DateTimeKind kind, bool isAmbiguousDst)
	{
		if (ticks < 0 || ticks > 3155378975999999999L)
		{
			throw new ArgumentOutOfRangeException("ticks", Environment.GetResourceString("Ticks must be between DateTime.MinValue.Ticks and DateTime.MaxValue.Ticks."));
		}
		dateData = (ulong)ticks | (isAmbiguousDst ? 13835058055282163712uL : 9223372036854775808uL);
	}

	public DateTime(int year, int month, int day)
	{
		dateData = (ulong)DateToTicks(year, month, day);
	}

	public DateTime(int year, int month, int day, Calendar calendar)
		: this(year, month, day, 0, 0, 0, calendar)
	{
	}

	public DateTime(int year, int month, int day, int hour, int minute, int second)
	{
		dateData = (ulong)(DateToTicks(year, month, day) + TimeToTicks(hour, minute, second));
	}

	public DateTime(int year, int month, int day, int hour, int minute, int second, DateTimeKind kind)
	{
		if (kind < DateTimeKind.Unspecified || kind > DateTimeKind.Local)
		{
			throw new ArgumentException(Environment.GetResourceString("Invalid DateTimeKind value."), "kind");
		}
		long num = DateToTicks(year, month, day) + TimeToTicks(hour, minute, second);
		dateData = (ulong)(num | ((long)kind << 62));
	}

	public DateTime(int year, int month, int day, int hour, int minute, int second, Calendar calendar)
	{
		if (calendar == null)
		{
			throw new ArgumentNullException("calendar");
		}
		dateData = (ulong)calendar.ToDateTime(year, month, day, hour, minute, second, 0).Ticks;
	}

	public DateTime(int year, int month, int day, int hour, int minute, int second, int millisecond)
	{
		if (millisecond < 0 || millisecond >= 1000)
		{
			throw new ArgumentOutOfRangeException("millisecond", Environment.GetResourceString("Valid values are between {0} and {1}, inclusive.", 0, 999));
		}
		long num = DateToTicks(year, month, day) + TimeToTicks(hour, minute, second);
		num += (long)millisecond * 10000L;
		if (num < 0 || num > 3155378975999999999L)
		{
			throw new ArgumentException(Environment.GetResourceString("Combination of arguments to the DateTime constructor is out of the legal range."));
		}
		dateData = (ulong)num;
	}

	public DateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, DateTimeKind kind)
	{
		if (millisecond < 0 || millisecond >= 1000)
		{
			throw new ArgumentOutOfRangeException("millisecond", Environment.GetResourceString("Valid values are between {0} and {1}, inclusive.", 0, 999));
		}
		if (kind < DateTimeKind.Unspecified || kind > DateTimeKind.Local)
		{
			throw new ArgumentException(Environment.GetResourceString("Invalid DateTimeKind value."), "kind");
		}
		long num = DateToTicks(year, month, day) + TimeToTicks(hour, minute, second);
		num += (long)millisecond * 10000L;
		if (num < 0 || num > 3155378975999999999L)
		{
			throw new ArgumentException(Environment.GetResourceString("Combination of arguments to the DateTime constructor is out of the legal range."));
		}
		dateData = (ulong)(num | ((long)kind << 62));
	}

	public DateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, Calendar calendar)
	{
		if (calendar == null)
		{
			throw new ArgumentNullException("calendar");
		}
		if (millisecond < 0 || millisecond >= 1000)
		{
			throw new ArgumentOutOfRangeException("millisecond", Environment.GetResourceString("Valid values are between {0} and {1}, inclusive.", 0, 999));
		}
		long ticks = calendar.ToDateTime(year, month, day, hour, minute, second, 0).Ticks;
		ticks += (long)millisecond * 10000L;
		if (ticks < 0 || ticks > 3155378975999999999L)
		{
			throw new ArgumentException(Environment.GetResourceString("Combination of arguments to the DateTime constructor is out of the legal range."));
		}
		dateData = (ulong)ticks;
	}

	public DateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, Calendar calendar, DateTimeKind kind)
	{
		if (calendar == null)
		{
			throw new ArgumentNullException("calendar");
		}
		if (millisecond < 0 || millisecond >= 1000)
		{
			throw new ArgumentOutOfRangeException("millisecond", Environment.GetResourceString("Valid values are between {0} and {1}, inclusive.", 0, 999));
		}
		if (kind < DateTimeKind.Unspecified || kind > DateTimeKind.Local)
		{
			throw new ArgumentException(Environment.GetResourceString("Invalid DateTimeKind value."), "kind");
		}
		long ticks = calendar.ToDateTime(year, month, day, hour, minute, second, 0).Ticks;
		ticks += (long)millisecond * 10000L;
		if (ticks < 0 || ticks > 3155378975999999999L)
		{
			throw new ArgumentException(Environment.GetResourceString("Combination of arguments to the DateTime constructor is out of the legal range."));
		}
		dateData = (ulong)(ticks | ((long)kind << 62));
	}

	private DateTime(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		bool flag = false;
		bool flag2 = false;
		long num = 0L;
		ulong num2 = 0uL;
		SerializationInfoEnumerator enumerator = info.GetEnumerator();
		while (enumerator.MoveNext())
		{
			string name = enumerator.Name;
			if (!(name == "ticks"))
			{
				if (name == "dateData")
				{
					num2 = Convert.ToUInt64(enumerator.Value, CultureInfo.InvariantCulture);
					flag2 = true;
				}
			}
			else
			{
				num = Convert.ToInt64(enumerator.Value, CultureInfo.InvariantCulture);
				flag = true;
			}
		}
		if (flag2)
		{
			dateData = num2;
		}
		else
		{
			if (!flag)
			{
				throw new SerializationException(Environment.GetResourceString("Invalid serialized DateTime data. Unable to find 'ticks' or 'dateData'."));
			}
			dateData = (ulong)num;
		}
		long internalTicks = InternalTicks;
		if (internalTicks < 0 || internalTicks > 3155378975999999999L)
		{
			throw new SerializationException(Environment.GetResourceString("Invalid serialized DateTime data. Ticks must be between DateTime.MinValue.Ticks and DateTime.MaxValue.Ticks."));
		}
	}

	public DateTime Add(TimeSpan value)
	{
		return AddTicks(value._ticks);
	}

	private DateTime Add(double value, int scale)
	{
		long num;
		try
		{
			num = checked((long)(value * (double)scale + ((value >= 0.0) ? 0.5 : (-0.5))));
		}
		catch (OverflowException)
		{
			throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("Value to add was out of range."));
		}
		if (num <= -315537897600000L || num >= 315537897600000L)
		{
			throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("Value to add was out of range."));
		}
		return AddTicks(num * 10000);
	}

	public DateTime AddDays(double value)
	{
		return Add(value, 86400000);
	}

	public DateTime AddHours(double value)
	{
		return Add(value, 3600000);
	}

	public DateTime AddMilliseconds(double value)
	{
		return Add(value, 1);
	}

	public DateTime AddMinutes(double value)
	{
		return Add(value, 60000);
	}

	public DateTime AddMonths(int months)
	{
		if (months < -120000 || months > 120000)
		{
			throw new ArgumentOutOfRangeException("months", Environment.GetResourceString("Months value must be between +/-120000."));
		}
		int datePart = GetDatePart(0);
		int datePart2 = GetDatePart(2);
		int num = GetDatePart(3);
		int num2 = datePart2 - 1 + months;
		if (num2 >= 0)
		{
			datePart2 = num2 % 12 + 1;
			datePart += num2 / 12;
		}
		else
		{
			datePart2 = 12 + (num2 + 1) % 12;
			datePart += (num2 - 11) / 12;
		}
		if (datePart < 1 || datePart > 9999)
		{
			throw new ArgumentOutOfRangeException("months", Environment.GetResourceString("The added or subtracted value results in an un-representable DateTime."));
		}
		int num3 = DaysInMonth(datePart, datePart2);
		if (num > num3)
		{
			num = num3;
		}
		return new DateTime((ulong)(DateToTicks(datePart, datePart2, num) + InternalTicks % 864000000000L) | InternalKind);
	}

	public DateTime AddSeconds(double value)
	{
		return Add(value, 1000);
	}

	public DateTime AddTicks(long value)
	{
		long internalTicks = InternalTicks;
		if (value > 3155378975999999999L - internalTicks || value < -internalTicks)
		{
			throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("The added or subtracted value results in an un-representable DateTime."));
		}
		return new DateTime((ulong)(internalTicks + value) | InternalKind);
	}

	public DateTime AddYears(int value)
	{
		if (value < -10000 || value > 10000)
		{
			throw new ArgumentOutOfRangeException("years", Environment.GetResourceString("Years value must be between +/-10000."));
		}
		return AddMonths(value * 12);
	}

	public static int Compare(DateTime t1, DateTime t2)
	{
		long internalTicks = t1.InternalTicks;
		long internalTicks2 = t2.InternalTicks;
		if (internalTicks > internalTicks2)
		{
			return 1;
		}
		if (internalTicks < internalTicks2)
		{
			return -1;
		}
		return 0;
	}

	public int CompareTo(object value)
	{
		if (value == null)
		{
			return 1;
		}
		if (!(value is DateTime { InternalTicks: var internalTicks }))
		{
			throw new ArgumentException(Environment.GetResourceString("Object must be of type DateTime."));
		}
		long internalTicks2 = InternalTicks;
		if (internalTicks2 > internalTicks)
		{
			return 1;
		}
		if (internalTicks2 < internalTicks)
		{
			return -1;
		}
		return 0;
	}

	public int CompareTo(DateTime value)
	{
		long internalTicks = value.InternalTicks;
		long internalTicks2 = InternalTicks;
		if (internalTicks2 > internalTicks)
		{
			return 1;
		}
		if (internalTicks2 < internalTicks)
		{
			return -1;
		}
		return 0;
	}

	private static long DateToTicks(int year, int month, int day)
	{
		if (year >= 1 && year <= 9999 && month >= 1 && month <= 12)
		{
			int[] array = (IsLeapYear(year) ? DaysToMonth366 : DaysToMonth365);
			if (day >= 1 && day <= array[month] - array[month - 1])
			{
				int num = year - 1;
				return (num * 365 + num / 4 - num / 100 + num / 400 + array[month - 1] + day - 1) * 864000000000L;
			}
		}
		throw new ArgumentOutOfRangeException(null, Environment.GetResourceString("Year, Month, and Day parameters describe an un-representable DateTime."));
	}

	private static long TimeToTicks(int hour, int minute, int second)
	{
		if (hour >= 0 && hour < 24 && minute >= 0 && minute < 60 && second >= 0 && second < 60)
		{
			return TimeSpan.TimeToTicks(hour, minute, second);
		}
		throw new ArgumentOutOfRangeException(null, Environment.GetResourceString("Hour, Minute, and Second parameters describe an un-representable DateTime."));
	}

	public static int DaysInMonth(int year, int month)
	{
		if (month < 1 || month > 12)
		{
			throw new ArgumentOutOfRangeException("month", Environment.GetResourceString("Month must be between one and twelve."));
		}
		int[] array = (IsLeapYear(year) ? DaysToMonth366 : DaysToMonth365);
		return array[month] - array[month - 1];
	}

	internal static long DoubleDateToTicks(double value)
	{
		if (!(value < 2958466.0) || !(value > -657435.0))
		{
			throw new ArgumentException(Environment.GetResourceString("Not a legal OleAut date."));
		}
		long num = (long)(value * 86400000.0 + ((value >= 0.0) ? 0.5 : (-0.5)));
		if (num < 0)
		{
			num -= num % 86400000 * 2;
		}
		num += 59926435200000L;
		if (num < 0 || num >= 315537897600000L)
		{
			throw new ArgumentException(Environment.GetResourceString("OleAut date did not convert to a DateTime correctly."));
		}
		return num * 10000;
	}

	public override bool Equals(object value)
	{
		if (value is DateTime)
		{
			return InternalTicks == ((DateTime)value).InternalTicks;
		}
		return false;
	}

	public bool Equals(DateTime value)
	{
		return InternalTicks == value.InternalTicks;
	}

	public static bool Equals(DateTime t1, DateTime t2)
	{
		return t1.InternalTicks == t2.InternalTicks;
	}

	public static DateTime FromBinary(long dateData)
	{
		if ((dateData & long.MinValue) != 0L)
		{
			long num = dateData & 0x3FFFFFFFFFFFFFFFL;
			if (num > 4611685154427387904L)
			{
				num -= 4611686018427387904L;
			}
			bool isAmbiguousLocalDst = false;
			long ticks;
			if (num < 0)
			{
				ticks = TimeZoneInfo.GetLocalUtcOffset(MinValue, TimeZoneInfoOptions.NoThrowOnInvalidTime).Ticks;
			}
			else if (num > 3155378975999999999L)
			{
				ticks = TimeZoneInfo.GetLocalUtcOffset(MaxValue, TimeZoneInfoOptions.NoThrowOnInvalidTime).Ticks;
			}
			else
			{
				DateTime time = new DateTime(num, DateTimeKind.Utc);
				bool isDaylightSavings = false;
				ticks = TimeZoneInfo.GetUtcOffsetFromUtc(time, TimeZoneInfo.Local, out isDaylightSavings, out isAmbiguousLocalDst).Ticks;
			}
			num += ticks;
			if (num < 0)
			{
				num += 864000000000L;
			}
			if (num < 0 || num > 3155378975999999999L)
			{
				throw new ArgumentException(Environment.GetResourceString("The binary data must result in a DateTime with ticks between DateTime.MinValue.Ticks and DateTime.MaxValue.Ticks."), "dateData");
			}
			return new DateTime(num, DateTimeKind.Local, isAmbiguousLocalDst);
		}
		return FromBinaryRaw(dateData);
	}

	internal static DateTime FromBinaryRaw(long dateData)
	{
		long num = dateData & 0x3FFFFFFFFFFFFFFFL;
		if (num < 0 || num > 3155378975999999999L)
		{
			throw new ArgumentException(Environment.GetResourceString("The binary data must result in a DateTime with ticks between DateTime.MinValue.Ticks and DateTime.MaxValue.Ticks."), "dateData");
		}
		return new DateTime((ulong)dateData);
	}

	public static DateTime FromFileTime(long fileTime)
	{
		return FromFileTimeUtc(fileTime).ToLocalTime();
	}

	public static DateTime FromFileTimeUtc(long fileTime)
	{
		if (fileTime < 0 || fileTime > 2650467743999999999L)
		{
			throw new ArgumentOutOfRangeException("fileTime", Environment.GetResourceString("Not a valid Win32 FileTime."));
		}
		return new DateTime(fileTime + 504911232000000000L, DateTimeKind.Utc);
	}

	public static DateTime FromOADate(double d)
	{
		return new DateTime(DoubleDateToTicks(d), DateTimeKind.Unspecified);
	}

	[SecurityCritical]
	void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		info.AddValue("ticks", InternalTicks);
		info.AddValue("dateData", dateData);
	}

	public bool IsDaylightSavingTime()
	{
		if (Kind == DateTimeKind.Utc)
		{
			return false;
		}
		return TimeZoneInfo.Local.IsDaylightSavingTime(this, TimeZoneInfoOptions.NoThrowOnInvalidTime);
	}

	public static DateTime SpecifyKind(DateTime value, DateTimeKind kind)
	{
		return new DateTime(value.InternalTicks, kind);
	}

	public long ToBinary()
	{
		if (Kind == DateTimeKind.Local)
		{
			TimeSpan localUtcOffset = TimeZoneInfo.GetLocalUtcOffset(this, TimeZoneInfoOptions.NoThrowOnInvalidTime);
			long num = Ticks - localUtcOffset.Ticks;
			if (num < 0)
			{
				num = 4611686018427387904L + num;
			}
			return num | long.MinValue;
		}
		return (long)dateData;
	}

	internal long ToBinaryRaw()
	{
		return (long)dateData;
	}

	private int GetDatePart(int part)
	{
		int num = (int)(InternalTicks / 864000000000L);
		int num2 = num / 146097;
		num -= num2 * 146097;
		int num3 = num / 36524;
		if (num3 == 4)
		{
			num3 = 3;
		}
		num -= num3 * 36524;
		int num4 = num / 1461;
		num -= num4 * 1461;
		int num5 = num / 365;
		if (num5 == 4)
		{
			num5 = 3;
		}
		if (part == 0)
		{
			return num2 * 400 + num3 * 100 + num4 * 4 + num5 + 1;
		}
		num -= num5 * 365;
		if (part == 1)
		{
			return num + 1;
		}
		int[] array = ((num5 == 3 && (num4 != 24 || num3 == 3)) ? DaysToMonth366 : DaysToMonth365);
		int i;
		for (i = num >> 6; num >= array[i]; i++)
		{
		}
		if (part == 2)
		{
			return i;
		}
		return num - array[i - 1] + 1;
	}

	public override int GetHashCode()
	{
		long internalTicks = InternalTicks;
		return (int)internalTicks ^ (int)(internalTicks >> 32);
	}

	internal bool IsAmbiguousDaylightSavingTime()
	{
		return InternalKind == 13835058055282163712uL;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SecurityCritical]
	internal static extern long GetSystemTimeAsFileTime();

	public static bool IsLeapYear(int year)
	{
		if (year < 1 || year > 9999)
		{
			throw new ArgumentOutOfRangeException("year", Environment.GetResourceString("Year must be between 1 and 9999."));
		}
		if (year % 4 == 0)
		{
			if (year % 100 == 0)
			{
				return year % 400 == 0;
			}
			return true;
		}
		return false;
	}

	public static DateTime Parse(string s)
	{
		return DateTimeParse.Parse(s, DateTimeFormatInfo.CurrentInfo, DateTimeStyles.None);
	}

	public static DateTime Parse(string s, IFormatProvider provider)
	{
		return DateTimeParse.Parse(s, DateTimeFormatInfo.GetInstance(provider), DateTimeStyles.None);
	}

	public static DateTime Parse(string s, IFormatProvider provider, DateTimeStyles styles)
	{
		DateTimeFormatInfo.ValidateStyles(styles, "styles");
		return DateTimeParse.Parse(s, DateTimeFormatInfo.GetInstance(provider), styles);
	}

	public static DateTime ParseExact(string s, string format, IFormatProvider provider)
	{
		return DateTimeParse.ParseExact(s, format, DateTimeFormatInfo.GetInstance(provider), DateTimeStyles.None);
	}

	public static DateTime ParseExact(string s, string format, IFormatProvider provider, DateTimeStyles style)
	{
		DateTimeFormatInfo.ValidateStyles(style, "style");
		return DateTimeParse.ParseExact(s, format, DateTimeFormatInfo.GetInstance(provider), style);
	}

	public static DateTime ParseExact(string s, string[] formats, IFormatProvider provider, DateTimeStyles style)
	{
		DateTimeFormatInfo.ValidateStyles(style, "style");
		return DateTimeParse.ParseExactMultiple(s, formats, DateTimeFormatInfo.GetInstance(provider), style);
	}

	public TimeSpan Subtract(DateTime value)
	{
		return new TimeSpan(InternalTicks - value.InternalTicks);
	}

	public DateTime Subtract(TimeSpan value)
	{
		long internalTicks = InternalTicks;
		long ticks = value._ticks;
		if (internalTicks < ticks || internalTicks - 3155378975999999999L > ticks)
		{
			throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("The added or subtracted value results in an un-representable DateTime."));
		}
		return new DateTime((ulong)(internalTicks - ticks) | InternalKind);
	}

	private static double TicksToOADate(long value)
	{
		if (value == 0L)
		{
			return 0.0;
		}
		if (value < 864000000000L)
		{
			value += 599264352000000000L;
		}
		if (value < 31241376000000000L)
		{
			throw new OverflowException(Environment.GetResourceString("Not a legal OleAut date."));
		}
		long num = (value - 599264352000000000L) / 10000;
		if (num < 0)
		{
			long num2 = num % 86400000;
			if (num2 != 0L)
			{
				num -= (86400000 + num2) * 2;
			}
		}
		return (double)num / 86400000.0;
	}

	public double ToOADate()
	{
		return TicksToOADate(InternalTicks);
	}

	public long ToFileTime()
	{
		return ToUniversalTime().ToFileTimeUtc();
	}

	public long ToFileTimeUtc()
	{
		long num = (((InternalKind & 0x8000000000000000uL) != 0L) ? ToUniversalTime().InternalTicks : InternalTicks) - 504911232000000000L;
		if (num < 0)
		{
			throw new ArgumentOutOfRangeException(null, Environment.GetResourceString("Not a valid Win32 FileTime."));
		}
		return num;
	}

	public DateTime ToLocalTime()
	{
		return ToLocalTime(throwOnOverflow: false);
	}

	internal DateTime ToLocalTime(bool throwOnOverflow)
	{
		if (Kind == DateTimeKind.Local)
		{
			return this;
		}
		bool isDaylightSavings = false;
		bool isAmbiguousLocalDst = false;
		long ticks = TimeZoneInfo.GetUtcOffsetFromUtc(this, TimeZoneInfo.Local, out isDaylightSavings, out isAmbiguousLocalDst).Ticks;
		long num = Ticks + ticks;
		if (num > 3155378975999999999L)
		{
			if (throwOnOverflow)
			{
				throw new ArgumentException(Environment.GetResourceString("Specified argument was out of the range of valid values."));
			}
			return new DateTime(3155378975999999999L, DateTimeKind.Local);
		}
		if (num < 0)
		{
			if (throwOnOverflow)
			{
				throw new ArgumentException(Environment.GetResourceString("Specified argument was out of the range of valid values."));
			}
			return new DateTime(0L, DateTimeKind.Local);
		}
		return new DateTime(num, DateTimeKind.Local, isAmbiguousLocalDst);
	}

	public string ToLongDateString()
	{
		return DateTimeFormat.Format(this, "D", DateTimeFormatInfo.CurrentInfo);
	}

	public string ToLongTimeString()
	{
		return DateTimeFormat.Format(this, "T", DateTimeFormatInfo.CurrentInfo);
	}

	public string ToShortDateString()
	{
		return DateTimeFormat.Format(this, "d", DateTimeFormatInfo.CurrentInfo);
	}

	public string ToShortTimeString()
	{
		return DateTimeFormat.Format(this, "t", DateTimeFormatInfo.CurrentInfo);
	}

	public override string ToString()
	{
		return DateTimeFormat.Format(this, null, DateTimeFormatInfo.CurrentInfo);
	}

	public string ToString(string format)
	{
		return DateTimeFormat.Format(this, format, DateTimeFormatInfo.CurrentInfo);
	}

	public string ToString(IFormatProvider provider)
	{
		return DateTimeFormat.Format(this, null, DateTimeFormatInfo.GetInstance(provider));
	}

	public string ToString(string format, IFormatProvider provider)
	{
		return DateTimeFormat.Format(this, format, DateTimeFormatInfo.GetInstance(provider));
	}

	public DateTime ToUniversalTime()
	{
		return TimeZoneInfo.ConvertTimeToUtc(this, TimeZoneInfoOptions.NoThrowOnInvalidTime);
	}

	public static bool TryParse(string s, out DateTime result)
	{
		return DateTimeParse.TryParse(s, DateTimeFormatInfo.CurrentInfo, DateTimeStyles.None, out result);
	}

	public static bool TryParse(string s, IFormatProvider provider, DateTimeStyles styles, out DateTime result)
	{
		DateTimeFormatInfo.ValidateStyles(styles, "styles");
		return DateTimeParse.TryParse(s, DateTimeFormatInfo.GetInstance(provider), styles, out result);
	}

	public static bool TryParseExact(string s, string format, IFormatProvider provider, DateTimeStyles style, out DateTime result)
	{
		DateTimeFormatInfo.ValidateStyles(style, "style");
		return DateTimeParse.TryParseExact(s, format, DateTimeFormatInfo.GetInstance(provider), style, out result);
	}

	public static bool TryParseExact(string s, string[] formats, IFormatProvider provider, DateTimeStyles style, out DateTime result)
	{
		DateTimeFormatInfo.ValidateStyles(style, "style");
		return DateTimeParse.TryParseExactMultiple(s, formats, DateTimeFormatInfo.GetInstance(provider), style, out result);
	}

	public static DateTime operator +(DateTime d, TimeSpan t)
	{
		long internalTicks = d.InternalTicks;
		long ticks = t._ticks;
		if (ticks > 3155378975999999999L - internalTicks || ticks < -internalTicks)
		{
			throw new ArgumentOutOfRangeException("t", Environment.GetResourceString("The added or subtracted value results in an un-representable DateTime."));
		}
		return new DateTime((ulong)(internalTicks + ticks) | d.InternalKind);
	}

	public static DateTime operator -(DateTime d, TimeSpan t)
	{
		long internalTicks = d.InternalTicks;
		long ticks = t._ticks;
		if (internalTicks < ticks || internalTicks - 3155378975999999999L > ticks)
		{
			throw new ArgumentOutOfRangeException("t", Environment.GetResourceString("The added or subtracted value results in an un-representable DateTime."));
		}
		return new DateTime((ulong)(internalTicks - ticks) | d.InternalKind);
	}

	public static TimeSpan operator -(DateTime d1, DateTime d2)
	{
		return new TimeSpan(d1.InternalTicks - d2.InternalTicks);
	}

	public static bool operator ==(DateTime d1, DateTime d2)
	{
		return d1.InternalTicks == d2.InternalTicks;
	}

	public static bool operator !=(DateTime d1, DateTime d2)
	{
		return d1.InternalTicks != d2.InternalTicks;
	}

	public static bool operator <(DateTime t1, DateTime t2)
	{
		return t1.InternalTicks < t2.InternalTicks;
	}

	public static bool operator <=(DateTime t1, DateTime t2)
	{
		return t1.InternalTicks <= t2.InternalTicks;
	}

	public static bool operator >(DateTime t1, DateTime t2)
	{
		return t1.InternalTicks > t2.InternalTicks;
	}

	public static bool operator >=(DateTime t1, DateTime t2)
	{
		return t1.InternalTicks >= t2.InternalTicks;
	}

	public string[] GetDateTimeFormats()
	{
		return GetDateTimeFormats(CultureInfo.CurrentCulture);
	}

	public string[] GetDateTimeFormats(IFormatProvider provider)
	{
		return DateTimeFormat.GetAllDateTimes(this, DateTimeFormatInfo.GetInstance(provider));
	}

	public string[] GetDateTimeFormats(char format)
	{
		return GetDateTimeFormats(format, CultureInfo.CurrentCulture);
	}

	public string[] GetDateTimeFormats(char format, IFormatProvider provider)
	{
		return DateTimeFormat.GetAllDateTimes(this, format, DateTimeFormatInfo.GetInstance(provider));
	}

	public TypeCode GetTypeCode()
	{
		return TypeCode.DateTime;
	}

	bool IConvertible.ToBoolean(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "DateTime", "Boolean"));
	}

	char IConvertible.ToChar(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "DateTime", "Char"));
	}

	sbyte IConvertible.ToSByte(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "DateTime", "SByte"));
	}

	byte IConvertible.ToByte(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "DateTime", "Byte"));
	}

	short IConvertible.ToInt16(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "DateTime", "Int16"));
	}

	ushort IConvertible.ToUInt16(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "DateTime", "UInt16"));
	}

	int IConvertible.ToInt32(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "DateTime", "Int32"));
	}

	uint IConvertible.ToUInt32(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "DateTime", "UInt32"));
	}

	long IConvertible.ToInt64(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "DateTime", "Int64"));
	}

	ulong IConvertible.ToUInt64(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "DateTime", "UInt64"));
	}

	float IConvertible.ToSingle(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "DateTime", "Single"));
	}

	double IConvertible.ToDouble(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "DateTime", "Double"));
	}

	decimal IConvertible.ToDecimal(IFormatProvider provider)
	{
		throw new InvalidCastException(Environment.GetResourceString("Invalid cast from '{0}' to '{1}'.", "DateTime", "Decimal"));
	}

	DateTime IConvertible.ToDateTime(IFormatProvider provider)
	{
		return this;
	}

	object IConvertible.ToType(Type type, IFormatProvider provider)
	{
		return Convert.DefaultToType(this, type, provider);
	}

	internal static bool TryCreate(int year, int month, int day, int hour, int minute, int second, int millisecond, out DateTime result)
	{
		result = MinValue;
		if (year < 1 || year > 9999 || month < 1 || month > 12)
		{
			return false;
		}
		int[] array = (IsLeapYear(year) ? DaysToMonth366 : DaysToMonth365);
		if (day < 1 || day > array[month] - array[month - 1])
		{
			return false;
		}
		if (hour < 0 || hour >= 24 || minute < 0 || minute >= 60 || second < 0 || second >= 60)
		{
			return false;
		}
		if (millisecond < 0 || millisecond >= 1000)
		{
			return false;
		}
		long num = DateToTicks(year, month, day) + TimeToTicks(hour, minute, second);
		num += (long)millisecond * 10000L;
		if (num < 0 || num > 3155378975999999999L)
		{
			return false;
		}
		result = new DateTime(num, DateTimeKind.Unspecified);
		return true;
	}
}
