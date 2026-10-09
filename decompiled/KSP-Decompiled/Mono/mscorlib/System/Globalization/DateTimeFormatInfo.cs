using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Runtime.Versioning;
using System.Security;
using System.Threading;

namespace System.Globalization;

[Serializable]
[ComVisible(true)]
public sealed class DateTimeFormatInfo : ICloneable, IFormatProvider
{
	private static volatile DateTimeFormatInfo invariantInfo;

	[NonSerialized]
	private CultureData m_cultureData;

	[OptionalField(VersionAdded = 2)]
	internal string m_name;

	[NonSerialized]
	private string m_langName;

	[NonSerialized]
	private CompareInfo m_compareInfo;

	[NonSerialized]
	private CultureInfo m_cultureInfo;

	internal string amDesignator;

	internal string pmDesignator;

	[OptionalField(VersionAdded = 1)]
	internal string dateSeparator;

	[OptionalField(VersionAdded = 1)]
	internal string generalShortTimePattern;

	[OptionalField(VersionAdded = 1)]
	internal string generalLongTimePattern;

	[OptionalField(VersionAdded = 1)]
	internal string timeSeparator;

	internal string monthDayPattern;

	[OptionalField(VersionAdded = 2)]
	internal string dateTimeOffsetPattern;

	internal const string rfc1123Pattern = "ddd, dd MMM yyyy HH':'mm':'ss 'GMT'";

	internal const string sortableDateTimePattern = "yyyy'-'MM'-'dd'T'HH':'mm':'ss";

	internal const string universalSortableDateTimePattern = "yyyy'-'MM'-'dd HH':'mm':'ss'Z'";

	internal Calendar calendar;

	internal int firstDayOfWeek = -1;

	internal int calendarWeekRule = -1;

	[NonSerialized]
	[OptionalField(VersionAdded = 1)]
	internal string fullDateTimePattern;

	internal string[] abbreviatedDayNames;

	[OptionalField(VersionAdded = 2)]
	internal string[] m_superShortDayNames;

	internal string[] dayNames;

	internal string[] abbreviatedMonthNames;

	internal string[] monthNames;

	[OptionalField(VersionAdded = 2)]
	internal string[] genitiveMonthNames;

	[OptionalField(VersionAdded = 2)]
	internal string[] m_genitiveAbbreviatedMonthNames;

	[OptionalField(VersionAdded = 2)]
	internal string[] leapYearMonthNames;

	internal string longDatePattern;

	internal string shortDatePattern;

	internal string yearMonthPattern;

	internal string longTimePattern;

	internal string shortTimePattern;

	[OptionalField(VersionAdded = 3)]
	private string[] allYearMonthPatterns;

	internal string[] allShortDatePatterns;

	internal string[] allLongDatePatterns;

	internal string[] allShortTimePatterns;

	internal string[] allLongTimePatterns;

	internal string[] m_eraNames;

	internal string[] m_abbrevEraNames;

	internal string[] m_abbrevEnglishEraNames;

	internal int[] optionalCalendars;

	private const int DEFAULT_ALL_DATETIMES_SIZE = 132;

	internal bool m_isReadOnly;

	[OptionalField(VersionAdded = 2)]
	internal DateTimeFormatFlags formatFlags = DateTimeFormatFlags.NotInitialized;

	internal static bool preferExistingTokens = InitPreferExistingTokens();

	[OptionalField(VersionAdded = 1)]
	private int CultureID;

	[OptionalField(VersionAdded = 1)]
	private bool m_useUserOverride;

	[OptionalField(VersionAdded = 1)]
	private bool bUseCalendarInfo;

	[OptionalField(VersionAdded = 1)]
	private int nDataItem;

	[OptionalField(VersionAdded = 2)]
	internal bool m_isDefaultCalendar;

	[OptionalField(VersionAdded = 2)]
	private static volatile Hashtable s_calendarNativeNames;

	[OptionalField(VersionAdded = 1)]
	internal string[] m_dateWords;

	[NonSerialized]
	private string m_fullTimeSpanPositivePattern;

	[NonSerialized]
	private string m_fullTimeSpanNegativePattern;

	internal const DateTimeStyles InvalidDateTimeStyles = ~(DateTimeStyles.AllowWhiteSpaces | DateTimeStyles.NoCurrentDateDefault | DateTimeStyles.AdjustToUniversal | DateTimeStyles.AssumeLocal | DateTimeStyles.AssumeUniversal | DateTimeStyles.RoundtripKind);

	[NonSerialized]
	private TokenHashValue[] m_dtfiTokenHash;

	private const int TOKEN_HASH_SIZE = 199;

	private const int SECOND_PRIME = 197;

	private const string dateSeparatorOrTimeZoneOffset = "-";

	private const string invariantDateSeparator = "/";

	private const string invariantTimeSeparator = ":";

	internal const string IgnorablePeriod = ".";

	internal const string IgnorableComma = ",";

	internal const string CJKYearSuff = "年";

	internal const string CJKMonthSuff = "月";

	internal const string CJKDaySuff = "日";

	internal const string KoreanYearSuff = "년";

	internal const string KoreanMonthSuff = "월";

	internal const string KoreanDaySuff = "일";

	internal const string KoreanHourSuff = "시";

	internal const string KoreanMinuteSuff = "분";

	internal const string KoreanSecondSuff = "초";

	internal const string CJKHourSuff = "時";

	internal const string ChineseHourSuff = "时";

	internal const string CJKMinuteSuff = "分";

	internal const string CJKSecondSuff = "秒";

	internal const string LocalTimeMark = "T";

	internal const string KoreanLangName = "ko";

	internal const string JapaneseLangName = "ja";

	internal const string EnglishLangName = "en";

	private static volatile DateTimeFormatInfo s_jajpDTFI;

	private static volatile DateTimeFormatInfo s_zhtwDTFI;

	private string CultureName
	{
		get
		{
			if (m_name == null)
			{
				m_name = m_cultureData.CultureName;
			}
			return m_name;
		}
	}

	private CultureInfo Culture
	{
		get
		{
			if (m_cultureInfo == null)
			{
				m_cultureInfo = CultureInfo.GetCultureInfo(CultureName);
			}
			return m_cultureInfo;
		}
	}

	private string LanguageName
	{
		[SecurityCritical]
		get
		{
			if (m_langName == null)
			{
				m_langName = m_cultureData.SISO639LANGNAME;
			}
			return m_langName;
		}
	}

	public static DateTimeFormatInfo InvariantInfo
	{
		get
		{
			if (invariantInfo == null)
			{
				DateTimeFormatInfo dateTimeFormatInfo = new DateTimeFormatInfo();
				dateTimeFormatInfo.Calendar.SetReadOnlyState(readOnly: true);
				dateTimeFormatInfo.m_isReadOnly = true;
				invariantInfo = dateTimeFormatInfo;
			}
			return invariantInfo;
		}
	}

	public static DateTimeFormatInfo CurrentInfo
	{
		get
		{
			CultureInfo currentCulture = Thread.CurrentThread.CurrentCulture;
			if (!currentCulture.m_isInherited)
			{
				DateTimeFormatInfo dateTimeInfo = currentCulture.dateTimeInfo;
				if (dateTimeInfo != null)
				{
					return dateTimeInfo;
				}
			}
			return (DateTimeFormatInfo)currentCulture.GetFormat(typeof(DateTimeFormatInfo));
		}
	}

	public string AMDesignator
	{
		get
		{
			return amDesignator;
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("String reference not set to an instance of a String."));
			}
			ClearTokenHashTable();
			amDesignator = value;
		}
	}

	public Calendar Calendar
	{
		get
		{
			return calendar;
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("Object cannot be null."));
			}
			if (value == calendar)
			{
				return;
			}
			CultureInfo.CheckDomainSafetyObject(value, this);
			for (int i = 0; i < OptionalCalendars.Length; i++)
			{
				if (OptionalCalendars[i] == value.ID)
				{
					if (calendar != null)
					{
						m_eraNames = null;
						m_abbrevEraNames = null;
						m_abbrevEnglishEraNames = null;
						monthDayPattern = null;
						dayNames = null;
						abbreviatedDayNames = null;
						m_superShortDayNames = null;
						monthNames = null;
						abbreviatedMonthNames = null;
						genitiveMonthNames = null;
						m_genitiveAbbreviatedMonthNames = null;
						leapYearMonthNames = null;
						formatFlags = DateTimeFormatFlags.NotInitialized;
						allShortDatePatterns = null;
						allLongDatePatterns = null;
						allYearMonthPatterns = null;
						dateTimeOffsetPattern = null;
						longDatePattern = null;
						shortDatePattern = null;
						yearMonthPattern = null;
						fullDateTimePattern = null;
						generalShortTimePattern = null;
						generalLongTimePattern = null;
						dateSeparator = null;
						ClearTokenHashTable();
					}
					calendar = value;
					InitializeOverridableProperties(m_cultureData, calendar.ID);
					return;
				}
			}
			throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("Not a valid calendar for the given culture."));
		}
	}

	private int[] OptionalCalendars
	{
		get
		{
			if (optionalCalendars == null)
			{
				optionalCalendars = m_cultureData.CalendarIds;
			}
			return optionalCalendars;
		}
	}

	internal string[] EraNames
	{
		get
		{
			if (m_eraNames == null)
			{
				m_eraNames = m_cultureData.EraNames(Calendar.ID);
			}
			return m_eraNames;
		}
	}

	internal string[] AbbreviatedEraNames
	{
		get
		{
			if (m_abbrevEraNames == null)
			{
				m_abbrevEraNames = m_cultureData.AbbrevEraNames(Calendar.ID);
			}
			return m_abbrevEraNames;
		}
	}

	internal string[] AbbreviatedEnglishEraNames
	{
		get
		{
			if (m_abbrevEnglishEraNames == null)
			{
				m_abbrevEnglishEraNames = m_cultureData.AbbreviatedEnglishEraNames(Calendar.ID);
			}
			return m_abbrevEnglishEraNames;
		}
	}

	public string DateSeparator
	{
		get
		{
			return dateSeparator;
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("String reference not set to an instance of a String."));
			}
			ClearTokenHashTable();
			dateSeparator = value;
		}
	}

	public DayOfWeek FirstDayOfWeek
	{
		get
		{
			return (DayOfWeek)firstDayOfWeek;
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value >= DayOfWeek.Sunday && value <= DayOfWeek.Saturday)
			{
				firstDayOfWeek = (int)value;
				return;
			}
			throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("Valid values are between {0} and {1}, inclusive.", DayOfWeek.Sunday, DayOfWeek.Saturday));
		}
	}

	public CalendarWeekRule CalendarWeekRule
	{
		get
		{
			return (CalendarWeekRule)calendarWeekRule;
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value >= CalendarWeekRule.FirstDay && value <= CalendarWeekRule.FirstFourDayWeek)
			{
				calendarWeekRule = (int)value;
				return;
			}
			throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("Valid values are between {0} and {1}, inclusive.", CalendarWeekRule.FirstDay, CalendarWeekRule.FirstFourDayWeek));
		}
	}

	public string FullDateTimePattern
	{
		get
		{
			if (fullDateTimePattern == null)
			{
				fullDateTimePattern = LongDatePattern + " " + LongTimePattern;
			}
			return fullDateTimePattern;
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("String reference not set to an instance of a String."));
			}
			fullDateTimePattern = value;
		}
	}

	public string LongDatePattern
	{
		get
		{
			if (longDatePattern == null)
			{
				longDatePattern = UnclonedLongDatePatterns[0];
			}
			return longDatePattern;
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("String reference not set to an instance of a String."));
			}
			longDatePattern = value;
			ClearTokenHashTable();
			fullDateTimePattern = null;
		}
	}

	public string LongTimePattern
	{
		get
		{
			if (longTimePattern == null)
			{
				longTimePattern = UnclonedLongTimePatterns[0];
			}
			return longTimePattern;
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("String reference not set to an instance of a String."));
			}
			longTimePattern = value;
			ClearTokenHashTable();
			fullDateTimePattern = null;
			generalLongTimePattern = null;
			dateTimeOffsetPattern = null;
		}
	}

	public string MonthDayPattern
	{
		get
		{
			if (monthDayPattern == null)
			{
				monthDayPattern = m_cultureData.MonthDay(Calendar.ID);
			}
			return monthDayPattern;
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("String reference not set to an instance of a String."));
			}
			monthDayPattern = value;
		}
	}

	public string PMDesignator
	{
		get
		{
			return pmDesignator;
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("String reference not set to an instance of a String."));
			}
			ClearTokenHashTable();
			pmDesignator = value;
		}
	}

	public string RFC1123Pattern => "ddd, dd MMM yyyy HH':'mm':'ss 'GMT'";

	public string ShortDatePattern
	{
		get
		{
			if (shortDatePattern == null)
			{
				shortDatePattern = UnclonedShortDatePatterns[0];
			}
			return shortDatePattern;
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("String reference not set to an instance of a String."));
			}
			shortDatePattern = value;
			ClearTokenHashTable();
			generalLongTimePattern = null;
			generalShortTimePattern = null;
			dateTimeOffsetPattern = null;
		}
	}

	public string ShortTimePattern
	{
		get
		{
			if (shortTimePattern == null)
			{
				shortTimePattern = UnclonedShortTimePatterns[0];
			}
			return shortTimePattern;
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("String reference not set to an instance of a String."));
			}
			shortTimePattern = value;
			ClearTokenHashTable();
			generalShortTimePattern = null;
		}
	}

	public string SortableDateTimePattern => "yyyy'-'MM'-'dd'T'HH':'mm':'ss";

	internal string GeneralShortTimePattern
	{
		get
		{
			if (generalShortTimePattern == null)
			{
				generalShortTimePattern = ShortDatePattern + " " + ShortTimePattern;
			}
			return generalShortTimePattern;
		}
	}

	internal string GeneralLongTimePattern
	{
		get
		{
			if (generalLongTimePattern == null)
			{
				generalLongTimePattern = ShortDatePattern + " " + LongTimePattern;
			}
			return generalLongTimePattern;
		}
	}

	internal string DateTimeOffsetPattern
	{
		get
		{
			if (dateTimeOffsetPattern == null)
			{
				dateTimeOffsetPattern = ShortDatePattern + " " + LongTimePattern;
				bool flag = false;
				bool flag2 = false;
				char c = '\'';
				int num = 0;
				while (!flag && num < LongTimePattern.Length)
				{
					switch (LongTimePattern[num])
					{
					case 'z':
						flag = !flag2;
						break;
					case '"':
					case '\'':
						if (flag2 && c == LongTimePattern[num])
						{
							flag2 = false;
						}
						else if (!flag2)
						{
							c = LongTimePattern[num];
							flag2 = true;
						}
						break;
					case '%':
					case '\\':
						num++;
						break;
					}
					num++;
				}
				if (!flag)
				{
					dateTimeOffsetPattern += " zzz";
				}
			}
			return dateTimeOffsetPattern;
		}
	}

	public string TimeSeparator
	{
		get
		{
			return timeSeparator;
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("String reference not set to an instance of a String."));
			}
			ClearTokenHashTable();
			timeSeparator = value;
		}
	}

	public string UniversalSortableDateTimePattern => "yyyy'-'MM'-'dd HH':'mm':'ss'Z'";

	public string YearMonthPattern
	{
		get
		{
			if (yearMonthPattern == null)
			{
				yearMonthPattern = UnclonedYearMonthPatterns[0];
			}
			return yearMonthPattern;
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("String reference not set to an instance of a String."));
			}
			yearMonthPattern = value;
			ClearTokenHashTable();
		}
	}

	public string[] AbbreviatedDayNames
	{
		get
		{
			return (string[])internalGetAbbreviatedDayOfWeekNames().Clone();
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("Array cannot be null."));
			}
			if (value.Length != 7)
			{
				throw new ArgumentException(Environment.GetResourceString("Length of the array must be {0}.", 7), "value");
			}
			CheckNullValue(value, value.Length);
			ClearTokenHashTable();
			abbreviatedDayNames = value;
		}
	}

	[ComVisible(false)]
	public string[] ShortestDayNames
	{
		get
		{
			return (string[])internalGetSuperShortDayNames().Clone();
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("Array cannot be null."));
			}
			if (value.Length != 7)
			{
				throw new ArgumentException(Environment.GetResourceString("Length of the array must be {0}.", 7), "value");
			}
			CheckNullValue(value, value.Length);
			m_superShortDayNames = value;
		}
	}

	public string[] DayNames
	{
		get
		{
			return (string[])internalGetDayOfWeekNames().Clone();
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("Array cannot be null."));
			}
			if (value.Length != 7)
			{
				throw new ArgumentException(Environment.GetResourceString("Length of the array must be {0}.", 7), "value");
			}
			CheckNullValue(value, value.Length);
			ClearTokenHashTable();
			dayNames = value;
		}
	}

	public string[] AbbreviatedMonthNames
	{
		get
		{
			return (string[])internalGetAbbreviatedMonthNames().Clone();
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("Array cannot be null."));
			}
			if (value.Length != 13)
			{
				throw new ArgumentException(Environment.GetResourceString("Length of the array must be {0}.", 13), "value");
			}
			CheckNullValue(value, value.Length - 1);
			ClearTokenHashTable();
			abbreviatedMonthNames = value;
		}
	}

	public string[] MonthNames
	{
		get
		{
			return (string[])internalGetMonthNames().Clone();
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("Array cannot be null."));
			}
			if (value.Length != 13)
			{
				throw new ArgumentException(Environment.GetResourceString("Length of the array must be {0}.", 13), "value");
			}
			CheckNullValue(value, value.Length - 1);
			monthNames = value;
			ClearTokenHashTable();
		}
	}

	internal bool HasSpacesInMonthNames => (FormatFlags & DateTimeFormatFlags.UseSpacesInMonthNames) != 0;

	internal bool HasSpacesInDayNames => (FormatFlags & DateTimeFormatFlags.UseSpacesInDayNames) != 0;

	private string[] AllYearMonthPatterns => GetMergedPatterns(UnclonedYearMonthPatterns, YearMonthPattern);

	private string[] AllShortDatePatterns => GetMergedPatterns(UnclonedShortDatePatterns, ShortDatePattern);

	private string[] AllShortTimePatterns => GetMergedPatterns(UnclonedShortTimePatterns, ShortTimePattern);

	private string[] AllLongDatePatterns => GetMergedPatterns(UnclonedLongDatePatterns, LongDatePattern);

	private string[] AllLongTimePatterns => GetMergedPatterns(UnclonedLongTimePatterns, LongTimePattern);

	private string[] UnclonedYearMonthPatterns
	{
		get
		{
			if (allYearMonthPatterns == null)
			{
				allYearMonthPatterns = m_cultureData.YearMonths(Calendar.ID);
			}
			return allYearMonthPatterns;
		}
	}

	private string[] UnclonedShortDatePatterns
	{
		get
		{
			if (allShortDatePatterns == null)
			{
				allShortDatePatterns = m_cultureData.ShortDates(Calendar.ID);
			}
			return allShortDatePatterns;
		}
	}

	private string[] UnclonedLongDatePatterns
	{
		get
		{
			if (allLongDatePatterns == null)
			{
				allLongDatePatterns = m_cultureData.LongDates(Calendar.ID);
			}
			return allLongDatePatterns;
		}
	}

	private string[] UnclonedShortTimePatterns
	{
		get
		{
			if (allShortTimePatterns == null)
			{
				allShortTimePatterns = m_cultureData.ShortTimes;
			}
			return allShortTimePatterns;
		}
	}

	private string[] UnclonedLongTimePatterns
	{
		get
		{
			if (allLongTimePatterns == null)
			{
				allLongTimePatterns = m_cultureData.LongTimes;
			}
			return allLongTimePatterns;
		}
	}

	public bool IsReadOnly => m_isReadOnly;

	[ComVisible(false)]
	public string NativeCalendarName => m_cultureData.CalendarName(Calendar.ID);

	[ComVisible(false)]
	public string[] AbbreviatedMonthGenitiveNames
	{
		get
		{
			return (string[])internalGetGenitiveMonthNames(abbreviated: true).Clone();
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("Array cannot be null."));
			}
			if (value.Length != 13)
			{
				throw new ArgumentException(Environment.GetResourceString("Length of the array must be {0}.", 13), "value");
			}
			CheckNullValue(value, value.Length - 1);
			ClearTokenHashTable();
			m_genitiveAbbreviatedMonthNames = value;
		}
	}

	[ComVisible(false)]
	public string[] MonthGenitiveNames
	{
		get
		{
			return (string[])internalGetGenitiveMonthNames(abbreviated: false).Clone();
		}
		set
		{
			if (IsReadOnly)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
			}
			if (value == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("Array cannot be null."));
			}
			if (value.Length != 13)
			{
				throw new ArgumentException(Environment.GetResourceString("Length of the array must be {0}.", 13), "value");
			}
			CheckNullValue(value, value.Length - 1);
			genitiveMonthNames = value;
			ClearTokenHashTable();
		}
	}

	internal string FullTimeSpanPositivePattern
	{
		get
		{
			if (m_fullTimeSpanPositivePattern == null)
			{
				CultureData cultureData = ((!m_cultureData.UseUserOverride) ? m_cultureData : CultureData.GetCultureData(m_cultureData.CultureName, useUserOverride: false));
				string numberDecimalSeparator = new NumberFormatInfo(cultureData).NumberDecimalSeparator;
				m_fullTimeSpanPositivePattern = "d':'h':'mm':'ss'" + numberDecimalSeparator + "'FFFFFFF";
			}
			return m_fullTimeSpanPositivePattern;
		}
	}

	internal string FullTimeSpanNegativePattern
	{
		get
		{
			if (m_fullTimeSpanNegativePattern == null)
			{
				m_fullTimeSpanNegativePattern = "'-'" + FullTimeSpanPositivePattern;
			}
			return m_fullTimeSpanNegativePattern;
		}
	}

	internal CompareInfo CompareInfo
	{
		get
		{
			if (m_compareInfo == null)
			{
				m_compareInfo = CompareInfo.GetCompareInfo(m_cultureData.SCOMPAREINFO);
			}
			return m_compareInfo;
		}
	}

	internal DateTimeFormatFlags FormatFlags
	{
		get
		{
			if (formatFlags == DateTimeFormatFlags.NotInitialized)
			{
				formatFlags = DateTimeFormatFlags.None;
				formatFlags |= (DateTimeFormatFlags)DateTimeFormatInfoScanner.GetFormatFlagGenitiveMonth(MonthNames, internalGetGenitiveMonthNames(abbreviated: false), AbbreviatedMonthNames, internalGetGenitiveMonthNames(abbreviated: true));
				formatFlags |= (DateTimeFormatFlags)DateTimeFormatInfoScanner.GetFormatFlagUseSpaceInMonthNames(MonthNames, internalGetGenitiveMonthNames(abbreviated: false), AbbreviatedMonthNames, internalGetGenitiveMonthNames(abbreviated: true));
				formatFlags |= (DateTimeFormatFlags)DateTimeFormatInfoScanner.GetFormatFlagUseSpaceInDayNames(DayNames, AbbreviatedDayNames);
				formatFlags |= (DateTimeFormatFlags)DateTimeFormatInfoScanner.GetFormatFlagUseHebrewCalendar(Calendar.ID);
			}
			return formatFlags;
		}
	}

	internal bool HasForceTwoDigitYears
	{
		get
		{
			int iD = calendar.ID;
			if ((uint)(iD - 3) <= 1u)
			{
				return true;
			}
			return false;
		}
	}

	internal bool HasYearMonthAdjustment => (FormatFlags & DateTimeFormatFlags.UseHebrewRule) != 0;

	[SecuritySafeCritical]
	private static bool InitPreferExistingTokens()
	{
		return false;
	}

	private string[] internalGetAbbreviatedDayOfWeekNames()
	{
		if (abbreviatedDayNames == null)
		{
			abbreviatedDayNames = m_cultureData.AbbreviatedDayNames(Calendar.ID);
		}
		return abbreviatedDayNames;
	}

	private string[] internalGetSuperShortDayNames()
	{
		if (m_superShortDayNames == null)
		{
			m_superShortDayNames = m_cultureData.SuperShortDayNames(Calendar.ID);
		}
		return m_superShortDayNames;
	}

	private string[] internalGetDayOfWeekNames()
	{
		if (dayNames == null)
		{
			dayNames = m_cultureData.DayNames(Calendar.ID);
		}
		return dayNames;
	}

	private string[] internalGetAbbreviatedMonthNames()
	{
		if (abbreviatedMonthNames == null)
		{
			abbreviatedMonthNames = m_cultureData.AbbreviatedMonthNames(Calendar.ID);
		}
		return abbreviatedMonthNames;
	}

	private string[] internalGetMonthNames()
	{
		if (monthNames == null)
		{
			monthNames = m_cultureData.MonthNames(Calendar.ID);
		}
		return monthNames;
	}

	public DateTimeFormatInfo()
		: this(CultureInfo.InvariantCulture.m_cultureData, GregorianCalendar.GetDefaultInstance())
	{
	}

	internal DateTimeFormatInfo(CultureData cultureData, Calendar cal)
	{
		m_cultureData = cultureData;
		Calendar = cal;
	}

	[SecuritySafeCritical]
	private void InitializeOverridableProperties(CultureData cultureData, int calendarID)
	{
		if (firstDayOfWeek == -1)
		{
			firstDayOfWeek = cultureData.IFIRSTDAYOFWEEK;
		}
		if (calendarWeekRule == -1)
		{
			calendarWeekRule = cultureData.IFIRSTWEEKOFYEAR;
		}
		if (amDesignator == null)
		{
			amDesignator = cultureData.SAM1159;
		}
		if (pmDesignator == null)
		{
			pmDesignator = cultureData.SPM2359;
		}
		if (timeSeparator == null)
		{
			timeSeparator = cultureData.TimeSeparator;
		}
		if (dateSeparator == null)
		{
			dateSeparator = cultureData.DateSeparator(calendarID);
		}
		allLongTimePatterns = m_cultureData.LongTimes;
		allShortTimePatterns = m_cultureData.ShortTimes;
		allLongDatePatterns = cultureData.LongDates(calendarID);
		allShortDatePatterns = cultureData.ShortDates(calendarID);
		allYearMonthPatterns = cultureData.YearMonths(calendarID);
	}

	[OnDeserialized]
	private void OnDeserialized(StreamingContext ctx)
	{
		if (m_name != null)
		{
			m_cultureData = CultureData.GetCultureData(m_name, m_useUserOverride);
			if (m_cultureData == null)
			{
				throw new CultureNotFoundException("m_name", m_name, Environment.GetResourceString("Culture is not supported."));
			}
		}
		else
		{
			m_cultureData = CultureData.GetCultureData(CultureID, m_useUserOverride);
		}
		if (calendar == null)
		{
			calendar = (Calendar)GregorianCalendar.GetDefaultInstance().Clone();
			calendar.SetReadOnlyState(m_isReadOnly);
		}
		else
		{
			CultureInfo.CheckDomainSafetyObject(calendar, this);
		}
		InitializeOverridableProperties(m_cultureData, calendar.ID);
		bool isReadOnly = m_isReadOnly;
		m_isReadOnly = false;
		if (longDatePattern != null)
		{
			LongDatePattern = longDatePattern;
		}
		if (shortDatePattern != null)
		{
			ShortDatePattern = shortDatePattern;
		}
		if (yearMonthPattern != null)
		{
			YearMonthPattern = yearMonthPattern;
		}
		if (longTimePattern != null)
		{
			LongTimePattern = longTimePattern;
		}
		if (shortTimePattern != null)
		{
			ShortTimePattern = shortTimePattern;
		}
		m_isReadOnly = isReadOnly;
	}

	[OnSerializing]
	private void OnSerializing(StreamingContext ctx)
	{
		CultureID = m_cultureData.ILANGUAGE;
		m_useUserOverride = m_cultureData.UseUserOverride;
		m_name = CultureName;
		if (s_calendarNativeNames == null)
		{
			s_calendarNativeNames = new Hashtable();
		}
		_ = LongTimePattern;
		_ = LongDatePattern;
		_ = ShortTimePattern;
		_ = ShortDatePattern;
		_ = YearMonthPattern;
		_ = AllLongTimePatterns;
		_ = AllLongDatePatterns;
		_ = AllShortTimePatterns;
		_ = AllShortDatePatterns;
		_ = AllYearMonthPatterns;
	}

	public static DateTimeFormatInfo GetInstance(IFormatProvider provider)
	{
		if (provider is CultureInfo { m_isInherited: false } cultureInfo)
		{
			return cultureInfo.DateTimeFormat;
		}
		if (provider is DateTimeFormatInfo result)
		{
			return result;
		}
		if (provider != null && provider.GetFormat(typeof(DateTimeFormatInfo)) is DateTimeFormatInfo result2)
		{
			return result2;
		}
		return CurrentInfo;
	}

	public object GetFormat(Type formatType)
	{
		if (!(formatType == typeof(DateTimeFormatInfo)))
		{
			return null;
		}
		return this;
	}

	public object Clone()
	{
		DateTimeFormatInfo obj = (DateTimeFormatInfo)MemberwiseClone();
		obj.calendar = (Calendar)Calendar.Clone();
		obj.m_isReadOnly = false;
		return obj;
	}

	public int GetEra(string eraName)
	{
		if (eraName == null)
		{
			throw new ArgumentNullException("eraName", Environment.GetResourceString("String reference not set to an instance of a String."));
		}
		if (eraName.Length == 0)
		{
			return -1;
		}
		for (int i = 0; i < EraNames.Length; i++)
		{
			if (m_eraNames[i].Length > 0 && string.Compare(eraName, m_eraNames[i], Culture, CompareOptions.IgnoreCase) == 0)
			{
				return i + 1;
			}
		}
		for (int j = 0; j < AbbreviatedEraNames.Length; j++)
		{
			if (string.Compare(eraName, m_abbrevEraNames[j], Culture, CompareOptions.IgnoreCase) == 0)
			{
				return j + 1;
			}
		}
		for (int k = 0; k < AbbreviatedEnglishEraNames.Length; k++)
		{
			if (string.Compare(eraName, m_abbrevEnglishEraNames[k], StringComparison.InvariantCultureIgnoreCase) == 0)
			{
				return k + 1;
			}
		}
		return -1;
	}

	public string GetEraName(int era)
	{
		if (era == 0)
		{
			era = Calendar.CurrentEraValue;
		}
		if (--era < EraNames.Length && era >= 0)
		{
			return m_eraNames[era];
		}
		throw new ArgumentOutOfRangeException("era", Environment.GetResourceString("Era value was not valid."));
	}

	public string GetAbbreviatedEraName(int era)
	{
		if (AbbreviatedEraNames.Length == 0)
		{
			return GetEraName(era);
		}
		if (era == 0)
		{
			era = Calendar.CurrentEraValue;
		}
		if (--era < m_abbrevEraNames.Length && era >= 0)
		{
			return m_abbrevEraNames[era];
		}
		throw new ArgumentOutOfRangeException("era", Environment.GetResourceString("Era value was not valid."));
	}

	private static void CheckNullValue(string[] values, int length)
	{
		for (int i = 0; i < length; i++)
		{
			if (values[i] == null)
			{
				throw new ArgumentNullException("value", Environment.GetResourceString("Found a null value within an array."));
			}
		}
	}

	internal string internalGetMonthName(int month, MonthNameStyles style, bool abbreviated)
	{
		string[] array = null;
		array = style switch
		{
			MonthNameStyles.Genitive => internalGetGenitiveMonthNames(abbreviated), 
			MonthNameStyles.LeapYear => internalGetLeapYearMonthNames(), 
			_ => abbreviated ? internalGetAbbreviatedMonthNames() : internalGetMonthNames(), 
		};
		if (month < 1 || month > array.Length)
		{
			throw new ArgumentOutOfRangeException("month", Environment.GetResourceString("Valid values are between {0} and {1}, inclusive.", 1, array.Length));
		}
		return array[month - 1];
	}

	private string[] internalGetGenitiveMonthNames(bool abbreviated)
	{
		if (abbreviated)
		{
			if (m_genitiveAbbreviatedMonthNames == null)
			{
				m_genitiveAbbreviatedMonthNames = m_cultureData.AbbreviatedGenitiveMonthNames(Calendar.ID);
			}
			return m_genitiveAbbreviatedMonthNames;
		}
		if (genitiveMonthNames == null)
		{
			genitiveMonthNames = m_cultureData.GenitiveMonthNames(Calendar.ID);
		}
		return genitiveMonthNames;
	}

	internal string[] internalGetLeapYearMonthNames()
	{
		if (leapYearMonthNames == null)
		{
			leapYearMonthNames = m_cultureData.LeapYearMonthNames(Calendar.ID);
		}
		return leapYearMonthNames;
	}

	public string GetAbbreviatedDayName(DayOfWeek dayofweek)
	{
		if (dayofweek < DayOfWeek.Sunday || dayofweek > DayOfWeek.Saturday)
		{
			throw new ArgumentOutOfRangeException("dayofweek", Environment.GetResourceString("Valid values are between {0} and {1}, inclusive.", DayOfWeek.Sunday, DayOfWeek.Saturday));
		}
		return internalGetAbbreviatedDayOfWeekNames()[(int)dayofweek];
	}

	[ComVisible(false)]
	public string GetShortestDayName(DayOfWeek dayOfWeek)
	{
		if (dayOfWeek < DayOfWeek.Sunday || dayOfWeek > DayOfWeek.Saturday)
		{
			throw new ArgumentOutOfRangeException("dayOfWeek", Environment.GetResourceString("Valid values are between {0} and {1}, inclusive.", DayOfWeek.Sunday, DayOfWeek.Saturday));
		}
		return internalGetSuperShortDayNames()[(int)dayOfWeek];
	}

	private static string[] GetCombinedPatterns(string[] patterns1, string[] patterns2, string connectString)
	{
		string[] array = new string[patterns1.Length * patterns2.Length];
		int num = 0;
		for (int i = 0; i < patterns1.Length; i++)
		{
			for (int j = 0; j < patterns2.Length; j++)
			{
				array[num++] = patterns1[i] + connectString + patterns2[j];
			}
		}
		return array;
	}

	public string[] GetAllDateTimePatterns()
	{
		List<string> list = new List<string>(132);
		for (int i = 0; i < DateTimeFormat.allStandardFormats.Length; i++)
		{
			string[] allDateTimePatterns = GetAllDateTimePatterns(DateTimeFormat.allStandardFormats[i]);
			for (int j = 0; j < allDateTimePatterns.Length; j++)
			{
				list.Add(allDateTimePatterns[j]);
			}
		}
		return list.ToArray();
	}

	public string[] GetAllDateTimePatterns(char format)
	{
		string[] array = null;
		switch (format)
		{
		case 'd':
			return AllShortDatePatterns;
		case 'D':
			return AllLongDatePatterns;
		case 'f':
			return GetCombinedPatterns(AllLongDatePatterns, AllShortTimePatterns, " ");
		case 'F':
		case 'U':
			return GetCombinedPatterns(AllLongDatePatterns, AllLongTimePatterns, " ");
		case 'g':
			return GetCombinedPatterns(AllShortDatePatterns, AllShortTimePatterns, " ");
		case 'G':
			return GetCombinedPatterns(AllShortDatePatterns, AllLongTimePatterns, " ");
		case 'M':
		case 'm':
			return new string[1] { MonthDayPattern };
		case 'O':
		case 'o':
			return new string[1] { "yyyy'-'MM'-'dd'T'HH':'mm':'ss.fffffffK" };
		case 'R':
		case 'r':
			return new string[1] { "ddd, dd MMM yyyy HH':'mm':'ss 'GMT'" };
		case 's':
			return new string[1] { "yyyy'-'MM'-'dd'T'HH':'mm':'ss" };
		case 't':
			return AllShortTimePatterns;
		case 'T':
			return AllLongTimePatterns;
		case 'u':
			return new string[1] { UniversalSortableDateTimePattern };
		case 'Y':
		case 'y':
			return AllYearMonthPatterns;
		default:
			throw new ArgumentException(Environment.GetResourceString("Format specifier was invalid."), "format");
		}
	}

	public string GetDayName(DayOfWeek dayofweek)
	{
		if (dayofweek < DayOfWeek.Sunday || dayofweek > DayOfWeek.Saturday)
		{
			throw new ArgumentOutOfRangeException("dayofweek", Environment.GetResourceString("Valid values are between {0} and {1}, inclusive.", DayOfWeek.Sunday, DayOfWeek.Saturday));
		}
		return internalGetDayOfWeekNames()[(int)dayofweek];
	}

	public string GetAbbreviatedMonthName(int month)
	{
		if (month < 1 || month > 13)
		{
			throw new ArgumentOutOfRangeException("month", Environment.GetResourceString("Valid values are between {0} and {1}, inclusive.", 1, 13));
		}
		return internalGetAbbreviatedMonthNames()[month - 1];
	}

	public string GetMonthName(int month)
	{
		if (month < 1 || month > 13)
		{
			throw new ArgumentOutOfRangeException("month", Environment.GetResourceString("Valid values are between {0} and {1}, inclusive.", 1, 13));
		}
		return internalGetMonthNames()[month - 1];
	}

	private static string[] GetMergedPatterns(string[] patterns, string defaultPattern)
	{
		if (defaultPattern == patterns[0])
		{
			return (string[])patterns.Clone();
		}
		int i;
		for (i = 0; i < patterns.Length && !(defaultPattern == patterns[i]); i++)
		{
		}
		string[] array;
		if (i < patterns.Length)
		{
			array = (string[])patterns.Clone();
			array[i] = array[0];
		}
		else
		{
			array = new string[patterns.Length + 1];
			Array.Copy(patterns, 0, array, 1, patterns.Length);
		}
		array[0] = defaultPattern;
		return array;
	}

	public static DateTimeFormatInfo ReadOnly(DateTimeFormatInfo dtfi)
	{
		if (dtfi == null)
		{
			throw new ArgumentNullException("dtfi", Environment.GetResourceString("Object cannot be null."));
		}
		if (dtfi.IsReadOnly)
		{
			return dtfi;
		}
		DateTimeFormatInfo obj = (DateTimeFormatInfo)dtfi.MemberwiseClone();
		obj.calendar = Calendar.ReadOnly(dtfi.Calendar);
		obj.m_isReadOnly = true;
		return obj;
	}

	[ComVisible(false)]
	public void SetAllDateTimePatterns(string[] patterns, char format)
	{
		if (IsReadOnly)
		{
			throw new InvalidOperationException(Environment.GetResourceString("Instance is read-only."));
		}
		if (patterns == null)
		{
			throw new ArgumentNullException("patterns", Environment.GetResourceString("Array cannot be null."));
		}
		if (patterns.Length == 0)
		{
			throw new ArgumentException(Environment.GetResourceString("Array must not be of length zero."), "patterns");
		}
		for (int i = 0; i < patterns.Length; i++)
		{
			if (patterns[i] == null)
			{
				throw new ArgumentNullException(Environment.GetResourceString("Found a null value within an array."));
			}
		}
		switch (format)
		{
		case 'd':
			allShortDatePatterns = patterns;
			shortDatePattern = allShortDatePatterns[0];
			break;
		case 'D':
			allLongDatePatterns = patterns;
			longDatePattern = allLongDatePatterns[0];
			break;
		case 't':
			allShortTimePatterns = patterns;
			shortTimePattern = allShortTimePatterns[0];
			break;
		case 'T':
			allLongTimePatterns = patterns;
			longTimePattern = allLongTimePatterns[0];
			break;
		case 'Y':
		case 'y':
			allYearMonthPatterns = patterns;
			yearMonthPattern = allYearMonthPatterns[0];
			break;
		default:
			throw new ArgumentException(Environment.GetResourceString("Format specifier was invalid."), "format");
		}
		ClearTokenHashTable();
	}

	internal static void ValidateStyles(DateTimeStyles style, string parameterName)
	{
		if (((uint)style & 0xFFFFFF00u) != 0)
		{
			throw new ArgumentException(Environment.GetResourceString("An undefined DateTimeStyles value is being used."), parameterName);
		}
		if ((style & DateTimeStyles.AssumeLocal) != 0 && (style & DateTimeStyles.AssumeUniversal) != 0)
		{
			throw new ArgumentException(Environment.GetResourceString("The DateTimeStyles values AssumeLocal and AssumeUniversal cannot be used together."), parameterName);
		}
		if ((style & DateTimeStyles.RoundtripKind) != 0 && (style & (DateTimeStyles.AdjustToUniversal | DateTimeStyles.AssumeLocal | DateTimeStyles.AssumeUniversal)) != 0)
		{
			throw new ArgumentException(Environment.GetResourceString("The DateTimeStyles value RoundtripKind cannot be used with the values AssumeLocal, AssumeUniversal or AdjustToUniversal."), parameterName);
		}
	}

	internal bool YearMonthAdjustment(ref int year, ref int month, bool parsedMonthName)
	{
		if ((FormatFlags & DateTimeFormatFlags.UseHebrewRule) != 0)
		{
			if (year < 1000)
			{
				year += 5000;
			}
			if (year < Calendar.GetYear(Calendar.MinSupportedDateTime) || year > Calendar.GetYear(Calendar.MaxSupportedDateTime))
			{
				return false;
			}
			if (parsedMonthName && !Calendar.IsLeapYear(year))
			{
				if (month >= 8)
				{
					month--;
				}
				else if (month == 7)
				{
					return false;
				}
			}
		}
		return true;
	}

	internal static DateTimeFormatInfo GetJapaneseCalendarDTFI()
	{
		DateTimeFormatInfo dateTimeFormat = s_jajpDTFI;
		if (dateTimeFormat == null)
		{
			dateTimeFormat = new CultureInfo("ja-JP", useUserOverride: false).DateTimeFormat;
			dateTimeFormat.Calendar = JapaneseCalendar.GetDefaultInstance();
			s_jajpDTFI = dateTimeFormat;
		}
		return dateTimeFormat;
	}

	internal static DateTimeFormatInfo GetTaiwanCalendarDTFI()
	{
		DateTimeFormatInfo dateTimeFormat = s_zhtwDTFI;
		if (dateTimeFormat == null)
		{
			dateTimeFormat = new CultureInfo("zh-TW", useUserOverride: false).DateTimeFormat;
			dateTimeFormat.Calendar = TaiwanCalendar.GetDefaultInstance();
			s_zhtwDTFI = dateTimeFormat;
		}
		return dateTimeFormat;
	}

	private void ClearTokenHashTable()
	{
		m_dtfiTokenHash = null;
		formatFlags = DateTimeFormatFlags.NotInitialized;
	}

	[SecurityCritical]
	internal TokenHashValue[] CreateTokenHashTable()
	{
		TokenHashValue[] array = m_dtfiTokenHash;
		if (array == null)
		{
			array = new TokenHashValue[199];
			bool num = LanguageName.Equals("ko");
			string text = TimeSeparator.Trim();
			if ("," != text)
			{
				InsertHash(array, ",", TokenType.IgnorableSymbol, 0);
			}
			if ("." != text)
			{
				InsertHash(array, ".", TokenType.IgnorableSymbol, 0);
			}
			if ("시" != text && "時" != text && "时" != text)
			{
				InsertHash(array, TimeSeparator, TokenType.SEP_Time, 0);
			}
			InsertHash(array, AMDesignator, (TokenType)1027, 0);
			InsertHash(array, PMDesignator, (TokenType)1284, 1);
			if (LanguageName.Equals("sq"))
			{
				InsertHash(array, "." + AMDesignator, (TokenType)1027, 0);
				InsertHash(array, "." + PMDesignator, (TokenType)1284, 1);
			}
			InsertHash(array, "年", TokenType.SEP_YearSuff, 0);
			InsertHash(array, "년", TokenType.SEP_YearSuff, 0);
			InsertHash(array, "月", TokenType.SEP_MonthSuff, 0);
			InsertHash(array, "월", TokenType.SEP_MonthSuff, 0);
			InsertHash(array, "日", TokenType.SEP_DaySuff, 0);
			InsertHash(array, "일", TokenType.SEP_DaySuff, 0);
			InsertHash(array, "時", TokenType.SEP_HourSuff, 0);
			InsertHash(array, "时", TokenType.SEP_HourSuff, 0);
			InsertHash(array, "分", TokenType.SEP_MinuteSuff, 0);
			InsertHash(array, "秒", TokenType.SEP_SecondSuff, 0);
			if (num)
			{
				InsertHash(array, "시", TokenType.SEP_HourSuff, 0);
				InsertHash(array, "분", TokenType.SEP_MinuteSuff, 0);
				InsertHash(array, "초", TokenType.SEP_SecondSuff, 0);
			}
			if (LanguageName.Equals("ky"))
			{
				InsertHash(array, "-", TokenType.IgnorableSymbol, 0);
			}
			else
			{
				InsertHash(array, "-", TokenType.SEP_DateOrOffset, 0);
			}
			string[] array2 = null;
			DateTimeFormatInfoScanner dateTimeFormatInfoScanner = null;
			dateTimeFormatInfoScanner = new DateTimeFormatInfoScanner();
			array2 = (m_dateWords = dateTimeFormatInfoScanner.GetDateWordsOfDTFI(this));
			_ = FormatFlags;
			bool flag = false;
			string text2 = null;
			if (array2 != null)
			{
				for (int i = 0; i < array2.Length; i++)
				{
					switch (array2[i][0])
					{
					case '\ue000':
						text2 = array2[i].Substring(1);
						AddMonthNames(array, text2);
						break;
					case '\ue001':
					{
						string text3 = array2[i].Substring(1);
						InsertHash(array, text3, TokenType.IgnorableSymbol, 0);
						if (DateSeparator.Trim(null).Equals(text3))
						{
							flag = true;
						}
						break;
					}
					default:
						InsertHash(array, array2[i], TokenType.DateWordToken, 0);
						if (LanguageName.Equals("eu"))
						{
							InsertHash(array, "." + array2[i], TokenType.DateWordToken, 0);
						}
						break;
					}
				}
			}
			if (!flag)
			{
				InsertHash(array, DateSeparator, TokenType.SEP_Date, 0);
			}
			AddMonthNames(array, null);
			for (int j = 1; j <= 13; j++)
			{
				InsertHash(array, GetAbbreviatedMonthName(j), TokenType.MonthToken, j);
			}
			if ((FormatFlags & DateTimeFormatFlags.UseGenitiveMonth) != 0)
			{
				for (int k = 1; k <= 13; k++)
				{
					string str = internalGetMonthName(k, MonthNameStyles.Genitive, abbreviated: false);
					InsertHash(array, str, TokenType.MonthToken, k);
				}
			}
			if ((FormatFlags & DateTimeFormatFlags.UseLeapYearMonth) != 0)
			{
				for (int l = 1; l <= 13; l++)
				{
					string str2 = internalGetMonthName(l, MonthNameStyles.LeapYear, abbreviated: false);
					InsertHash(array, str2, TokenType.MonthToken, l);
				}
			}
			for (int m = 0; m < 7; m++)
			{
				string dayName = GetDayName((DayOfWeek)m);
				InsertHash(array, dayName, TokenType.DayOfWeekToken, m);
				dayName = GetAbbreviatedDayName((DayOfWeek)m);
				InsertHash(array, dayName, TokenType.DayOfWeekToken, m);
			}
			int[] eras = calendar.Eras;
			for (int n = 1; n <= eras.Length; n++)
			{
				InsertHash(array, GetEraName(n), TokenType.EraToken, n);
				InsertHash(array, GetAbbreviatedEraName(n), TokenType.EraToken, n);
			}
			if (LanguageName.Equals("ja"))
			{
				for (int num2 = 0; num2 < 7; num2++)
				{
					string str3 = "(" + GetAbbreviatedDayName((DayOfWeek)num2) + ")";
					InsertHash(array, str3, TokenType.DayOfWeekToken, num2);
				}
				if (Calendar.GetType() != typeof(JapaneseCalendar))
				{
					DateTimeFormatInfo japaneseCalendarDTFI = GetJapaneseCalendarDTFI();
					for (int num3 = 1; num3 <= japaneseCalendarDTFI.Calendar.Eras.Length; num3++)
					{
						InsertHash(array, japaneseCalendarDTFI.GetEraName(num3), TokenType.JapaneseEraToken, num3);
						InsertHash(array, japaneseCalendarDTFI.GetAbbreviatedEraName(num3), TokenType.JapaneseEraToken, num3);
						InsertHash(array, japaneseCalendarDTFI.AbbreviatedEnglishEraNames[num3 - 1], TokenType.JapaneseEraToken, num3);
					}
				}
			}
			else if (CultureName.Equals("zh-TW"))
			{
				DateTimeFormatInfo taiwanCalendarDTFI = GetTaiwanCalendarDTFI();
				for (int num4 = 1; num4 <= taiwanCalendarDTFI.Calendar.Eras.Length; num4++)
				{
					if (taiwanCalendarDTFI.GetEraName(num4).Length > 0)
					{
						InsertHash(array, taiwanCalendarDTFI.GetEraName(num4), TokenType.TEraToken, num4);
					}
				}
			}
			InsertHash(array, InvariantInfo.AMDesignator, (TokenType)1027, 0);
			InsertHash(array, InvariantInfo.PMDesignator, (TokenType)1284, 1);
			for (int num5 = 1; num5 <= 12; num5++)
			{
				string monthName = InvariantInfo.GetMonthName(num5);
				InsertHash(array, monthName, TokenType.MonthToken, num5);
				monthName = InvariantInfo.GetAbbreviatedMonthName(num5);
				InsertHash(array, monthName, TokenType.MonthToken, num5);
			}
			for (int num6 = 0; num6 < 7; num6++)
			{
				string dayName2 = InvariantInfo.GetDayName((DayOfWeek)num6);
				InsertHash(array, dayName2, TokenType.DayOfWeekToken, num6);
				dayName2 = InvariantInfo.GetAbbreviatedDayName((DayOfWeek)num6);
				InsertHash(array, dayName2, TokenType.DayOfWeekToken, num6);
			}
			for (int num7 = 0; num7 < AbbreviatedEnglishEraNames.Length; num7++)
			{
				InsertHash(array, AbbreviatedEnglishEraNames[num7], TokenType.EraToken, num7 + 1);
			}
			InsertHash(array, "T", TokenType.SEP_LocalTimeMark, 0);
			InsertHash(array, "GMT", TokenType.TimeZoneToken, 0);
			InsertHash(array, "Z", TokenType.TimeZoneToken, 0);
			InsertHash(array, "/", TokenType.SEP_Date, 0);
			InsertHash(array, ":", TokenType.SEP_Time, 0);
			m_dtfiTokenHash = array;
		}
		return array;
	}

	private void AddMonthNames(TokenHashValue[] temp, string monthPostfix)
	{
		for (int i = 1; i <= 13; i++)
		{
			string monthName = GetMonthName(i);
			if (monthName.Length > 0)
			{
				if (monthPostfix != null)
				{
					InsertHash(temp, monthName + monthPostfix, TokenType.MonthToken, i);
				}
				else
				{
					InsertHash(temp, monthName, TokenType.MonthToken, i);
				}
			}
			monthName = GetAbbreviatedMonthName(i);
			InsertHash(temp, monthName, TokenType.MonthToken, i);
		}
	}

	private static bool TryParseHebrewNumber(ref __DTString str, out bool badFormat, out int number)
	{
		number = -1;
		badFormat = false;
		int index = str.Index;
		if (!HebrewNumber.IsDigit(str.Value[index]))
		{
			return false;
		}
		HebrewNumberParsingContext context = new HebrewNumberParsingContext(0);
		HebrewNumberParsingState hebrewNumberParsingState;
		do
		{
			hebrewNumberParsingState = HebrewNumber.ParseByChar(str.Value[index++], ref context);
			if ((uint)hebrewNumberParsingState <= 1u)
			{
				return false;
			}
		}
		while (index < str.Value.Length && hebrewNumberParsingState != HebrewNumberParsingState.FoundEndOfHebrewNumber);
		if (hebrewNumberParsingState != HebrewNumberParsingState.FoundEndOfHebrewNumber)
		{
			return false;
		}
		str.Advance(index - str.Index);
		number = context.result;
		return true;
	}

	private static bool IsHebrewChar(char ch)
	{
		if (ch >= '\u0590')
		{
			return ch <= '\u05ff';
		}
		return false;
	}

	[SecurityCritical]
	internal bool Tokenize(TokenType TokenMask, out TokenType tokenType, out int tokenValue, ref __DTString str)
	{
		tokenType = TokenType.UnknownToken;
		tokenValue = 0;
		char c = str.m_current;
		bool flag = char.IsLetter(c);
		if (flag)
		{
			c = char.ToLower(c, Culture);
			if (IsHebrewChar(c) && TokenMask == TokenType.RegularTokenMask && TryParseHebrewNumber(ref str, out var badFormat, out tokenValue))
			{
				if (badFormat)
				{
					tokenType = TokenType.UnknownToken;
					return false;
				}
				tokenType = TokenType.HebrewNumber;
				return true;
			}
		}
		int num = c % 199;
		int num2 = 1 + c % 197;
		int num3 = str.len - str.Index;
		int num4 = 0;
		TokenHashValue[] array = m_dtfiTokenHash;
		if (array == null)
		{
			array = CreateTokenHashTable();
		}
		do
		{
			TokenHashValue tokenHashValue = array[num];
			if (tokenHashValue == null)
			{
				break;
			}
			if ((tokenHashValue.tokenType & TokenMask) > (TokenType)0 && tokenHashValue.tokenString.Length <= num3)
			{
				if (string.Compare(str.Value, str.Index, tokenHashValue.tokenString, 0, tokenHashValue.tokenString.Length, Culture, CompareOptions.IgnoreCase) == 0)
				{
					int index;
					if (flag && (index = str.Index + tokenHashValue.tokenString.Length) < str.len && char.IsLetter(str.Value[index]))
					{
						return false;
					}
					tokenType = tokenHashValue.tokenType & TokenMask;
					tokenValue = tokenHashValue.tokenValue;
					str.Advance(tokenHashValue.tokenString.Length);
					return true;
				}
				if (tokenHashValue.tokenType == TokenType.MonthToken && HasSpacesInMonthNames)
				{
					int matchLength = 0;
					if (str.MatchSpecifiedWords(tokenHashValue.tokenString, checkWordBoundary: true, ref matchLength))
					{
						tokenType = tokenHashValue.tokenType & TokenMask;
						tokenValue = tokenHashValue.tokenValue;
						str.Advance(matchLength);
						return true;
					}
				}
				else if (tokenHashValue.tokenType == TokenType.DayOfWeekToken && HasSpacesInDayNames)
				{
					int matchLength2 = 0;
					if (str.MatchSpecifiedWords(tokenHashValue.tokenString, checkWordBoundary: true, ref matchLength2))
					{
						tokenType = tokenHashValue.tokenType & TokenMask;
						tokenValue = tokenHashValue.tokenValue;
						str.Advance(matchLength2);
						return true;
					}
				}
			}
			num4++;
			num += num2;
			if (num >= 199)
			{
				num -= 199;
			}
		}
		while (num4 < 199);
		return false;
	}

	private void InsertAtCurrentHashNode(TokenHashValue[] hashTable, string str, char ch, TokenType tokenType, int tokenValue, int pos, int hashcode, int hashProbe)
	{
		TokenHashValue tokenHashValue = hashTable[hashcode];
		hashTable[hashcode] = new TokenHashValue(str, tokenType, tokenValue);
		while (++pos < 199)
		{
			hashcode += hashProbe;
			if (hashcode >= 199)
			{
				hashcode -= 199;
			}
			TokenHashValue tokenHashValue2 = hashTable[hashcode];
			if (tokenHashValue2 == null || char.ToLower(tokenHashValue2.tokenString[0], Culture) == ch)
			{
				hashTable[hashcode] = tokenHashValue;
				if (tokenHashValue2 == null)
				{
					break;
				}
				tokenHashValue = tokenHashValue2;
			}
		}
	}

	private void InsertHash(TokenHashValue[] hashTable, string str, TokenType tokenType, int tokenValue)
	{
		if (str == null || str.Length == 0)
		{
			return;
		}
		int num = 0;
		if (char.IsWhiteSpace(str[0]) || char.IsWhiteSpace(str[str.Length - 1]))
		{
			str = str.Trim(null);
			if (str.Length == 0)
			{
				return;
			}
		}
		char c = char.ToLower(str[0], Culture);
		int num2 = c % 199;
		int num3 = 1 + c % 197;
		do
		{
			TokenHashValue tokenHashValue = hashTable[num2];
			if (tokenHashValue == null)
			{
				hashTable[num2] = new TokenHashValue(str, tokenType, tokenValue);
				break;
			}
			if (str.Length >= tokenHashValue.tokenString.Length && string.Compare(str, 0, tokenHashValue.tokenString, 0, tokenHashValue.tokenString.Length, Culture, CompareOptions.IgnoreCase) == 0)
			{
				if (str.Length > tokenHashValue.tokenString.Length)
				{
					InsertAtCurrentHashNode(hashTable, str, c, tokenType, tokenValue, num, num2, num3);
					break;
				}
				int tokenType2 = (int)tokenHashValue.tokenType;
				if (preferExistingTokens || BinaryCompatibility.TargetsAtLeast_Desktop_V4_5_1)
				{
					if (((tokenType2 & 0xFF) == 0 && (tokenType & TokenType.RegularTokenMask) != 0) || ((tokenType2 & 0xFF00) == 0 && (tokenType & TokenType.SeparatorTokenMask) != 0))
					{
						tokenHashValue.tokenType |= tokenType;
						if (tokenValue != 0)
						{
							tokenHashValue.tokenValue = tokenValue;
						}
					}
				}
				else if ((((uint)tokenType | (uint)tokenType2) & 0xFF) == (uint)tokenType || (((uint)tokenType | (uint)tokenType2) & 0xFF00) == (uint)tokenType)
				{
					tokenHashValue.tokenType |= tokenType;
					if (tokenValue != 0)
					{
						tokenHashValue.tokenValue = tokenValue;
					}
				}
			}
			num++;
			num2 += num3;
			if (num2 >= 199)
			{
				num2 -= 199;
			}
		}
		while (num < 199);
	}
}
