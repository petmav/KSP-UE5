using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace System.Globalization;

[StructLayout(LayoutKind.Sequential)]
internal class CultureData
{
	private string sAM1159;

	private string sPM2359;

	private string sTimeSeparator;

	private volatile string[] saLongTimes;

	private volatile string[] saShortTimes;

	private int iFirstDayOfWeek;

	private int iFirstWeekOfYear;

	private volatile int[] waCalendars;

	private CalendarData[] calendars;

	private string sISO639Language;

	private readonly string sRealName;

	private bool bUseOverrides;

	private int calendarId;

	private int numberIndex;

	private int iDefaultAnsiCodePage;

	private int iDefaultOemCodePage;

	private int iDefaultMacCodePage;

	private int iDefaultEbcdicCodePage;

	private bool isRightToLeft;

	private string sListSeparator;

	private static CultureData s_Invariant;

	public static CultureData Invariant
	{
		get
		{
			if (s_Invariant == null)
			{
				CultureData cultureData = new CultureData("");
				cultureData.sISO639Language = "iv";
				cultureData.sAM1159 = "AM";
				cultureData.sPM2359 = "PM";
				cultureData.sTimeSeparator = ":";
				cultureData.saLongTimes = new string[1] { "HH:mm:ss" };
				cultureData.saShortTimes = new string[4] { "HH:mm", "hh:mm tt", "H:mm", "h:mm tt" };
				cultureData.iFirstDayOfWeek = 0;
				cultureData.iFirstWeekOfYear = 0;
				cultureData.waCalendars = new int[1] { 1 };
				cultureData.calendars = new CalendarData[23];
				cultureData.calendars[0] = CalendarData.Invariant;
				cultureData.iDefaultAnsiCodePage = 1252;
				cultureData.iDefaultOemCodePage = 437;
				cultureData.iDefaultMacCodePage = 10000;
				cultureData.iDefaultEbcdicCodePage = 37;
				cultureData.sListSeparator = ",";
				Interlocked.CompareExchange(ref s_Invariant, cultureData, null);
			}
			return s_Invariant;
		}
	}

	internal string[] LongTimes => saLongTimes;

	internal string[] ShortTimes => saShortTimes;

	internal string SISO639LANGNAME => sISO639Language;

	internal int IFIRSTDAYOFWEEK => iFirstDayOfWeek;

	internal int IFIRSTWEEKOFYEAR => iFirstWeekOfYear;

	internal string SAM1159 => sAM1159;

	internal string SPM2359 => sPM2359;

	internal string TimeSeparator => sTimeSeparator;

	internal int[] CalendarIds
	{
		get
		{
			if (waCalendars == null)
			{
				string text = sISO639Language;
				if (!(text == "ja"))
				{
					if (text == "zh")
					{
						waCalendars = new int[2] { calendarId, 4 };
					}
					else
					{
						waCalendars = new int[1] { calendarId };
					}
				}
				else
				{
					waCalendars = new int[2] { calendarId, 3 };
				}
			}
			return waCalendars;
		}
	}

	internal bool IsInvariantCulture => string.IsNullOrEmpty(sRealName);

	internal string CultureName => sRealName;

	internal string SCOMPAREINFO => "";

	internal string STEXTINFO => sRealName;

	internal int ILANGUAGE => 0;

	internal int IDEFAULTANSICODEPAGE => iDefaultAnsiCodePage;

	internal int IDEFAULTOEMCODEPAGE => iDefaultOemCodePage;

	internal int IDEFAULTMACCODEPAGE => iDefaultMacCodePage;

	internal int IDEFAULTEBCDICCODEPAGE => iDefaultEbcdicCodePage;

	internal bool IsRightToLeft => isRightToLeft;

	internal string SLIST => sListSeparator;

	internal bool UseUserOverride => bUseOverrides;

	private CultureData(string name)
	{
		sRealName = name;
	}

	public static CultureData GetCultureData(string cultureName, bool useUserOverride)
	{
		try
		{
			return new CultureInfo(cultureName, useUserOverride).m_cultureData;
		}
		catch
		{
			return null;
		}
	}

	public static CultureData GetCultureData(string cultureName, bool useUserOverride, int datetimeIndex, int calendarId, int numberIndex, string iso2lang, int ansiCodePage, int oemCodePage, int macCodePage, int ebcdicCodePage, bool rightToLeft, string listSeparator)
	{
		if (string.IsNullOrEmpty(cultureName))
		{
			return Invariant;
		}
		CultureData cultureData = new CultureData(cultureName);
		cultureData.fill_culture_data(datetimeIndex);
		cultureData.bUseOverrides = useUserOverride;
		cultureData.calendarId = calendarId;
		cultureData.numberIndex = numberIndex;
		cultureData.sISO639Language = iso2lang;
		cultureData.iDefaultAnsiCodePage = ansiCodePage;
		cultureData.iDefaultOemCodePage = oemCodePage;
		cultureData.iDefaultMacCodePage = macCodePage;
		cultureData.iDefaultEbcdicCodePage = ebcdicCodePage;
		cultureData.isRightToLeft = rightToLeft;
		cultureData.sListSeparator = listSeparator;
		return cultureData;
	}

	internal static CultureData GetCultureData(int culture, bool bUseUserOverride)
	{
		return null;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern void fill_culture_data(int datetimeIndex);

	public CalendarData GetCalendar(int calendarId)
	{
		int num = calendarId - 1;
		if (calendars == null)
		{
			calendars = new CalendarData[23];
		}
		CalendarData calendarData = calendars[num];
		if (calendarData == null)
		{
			calendarData = new CalendarData(sRealName, calendarId, bUseOverrides);
			calendars[num] = calendarData;
		}
		return calendarData;
	}

	internal string CalendarName(int calendarId)
	{
		return GetCalendar(calendarId).sNativeName;
	}

	internal string[] EraNames(int calendarId)
	{
		return GetCalendar(calendarId).saEraNames;
	}

	internal string[] AbbrevEraNames(int calendarId)
	{
		return GetCalendar(calendarId).saAbbrevEraNames;
	}

	internal string[] AbbreviatedEnglishEraNames(int calendarId)
	{
		return GetCalendar(calendarId).saAbbrevEnglishEraNames;
	}

	internal string[] ShortDates(int calendarId)
	{
		return GetCalendar(calendarId).saShortDates;
	}

	internal string[] LongDates(int calendarId)
	{
		return GetCalendar(calendarId).saLongDates;
	}

	internal string[] YearMonths(int calendarId)
	{
		return GetCalendar(calendarId).saYearMonths;
	}

	internal string[] DayNames(int calendarId)
	{
		return GetCalendar(calendarId).saDayNames;
	}

	internal string[] AbbreviatedDayNames(int calendarId)
	{
		return GetCalendar(calendarId).saAbbrevDayNames;
	}

	internal string[] SuperShortDayNames(int calendarId)
	{
		return GetCalendar(calendarId).saSuperShortDayNames;
	}

	internal string[] MonthNames(int calendarId)
	{
		return GetCalendar(calendarId).saMonthNames;
	}

	internal string[] GenitiveMonthNames(int calendarId)
	{
		return GetCalendar(calendarId).saMonthGenitiveNames;
	}

	internal string[] AbbreviatedMonthNames(int calendarId)
	{
		return GetCalendar(calendarId).saAbbrevMonthNames;
	}

	internal string[] AbbreviatedGenitiveMonthNames(int calendarId)
	{
		return GetCalendar(calendarId).saAbbrevMonthGenitiveNames;
	}

	internal string[] LeapYearMonthNames(int calendarId)
	{
		return GetCalendar(calendarId).saLeapYearMonthNames;
	}

	internal string MonthDay(int calendarId)
	{
		return GetCalendar(calendarId).sMonthDay;
	}

	internal string DateSeparator(int calendarId)
	{
		return GetDateSeparator(ShortDates(calendarId)[0]);
	}

	private static string GetDateSeparator(string format)
	{
		return GetSeparator(format, "dyM");
	}

	private static string GetSeparator(string format, string timeParts)
	{
		int num = IndexOfTimePart(format, 0, timeParts);
		if (num != -1)
		{
			char c = format[num];
			do
			{
				num++;
			}
			while (num < format.Length && format[num] == c);
			int num2 = num;
			if (num2 < format.Length)
			{
				int num3 = IndexOfTimePart(format, num2, timeParts);
				if (num3 != -1)
				{
					return UnescapeNlsString(format, num2, num3 - 1);
				}
			}
		}
		return string.Empty;
	}

	private static int IndexOfTimePart(string format, int startIndex, string timeParts)
	{
		bool flag = false;
		for (int i = startIndex; i < format.Length; i++)
		{
			if (!flag && timeParts.IndexOf(format[i]) != -1)
			{
				return i;
			}
			switch (format[i])
			{
			case '\\':
				if (i + 1 < format.Length)
				{
					i++;
					char c = format[i];
					if (c != '\'' && c != '\\')
					{
						i--;
					}
				}
				break;
			case '\'':
				flag = !flag;
				break;
			}
		}
		return -1;
	}

	private static string UnescapeNlsString(string str, int start, int end)
	{
		StringBuilder stringBuilder = null;
		for (int i = start; i < str.Length && i <= end; i++)
		{
			switch (str[i])
			{
			case '\'':
				if (stringBuilder == null)
				{
					stringBuilder = new StringBuilder(str, start, i - start, str.Length);
				}
				break;
			case '\\':
				if (stringBuilder == null)
				{
					stringBuilder = new StringBuilder(str, start, i - start, str.Length);
				}
				i++;
				if (i < str.Length)
				{
					stringBuilder.Append(str[i]);
				}
				break;
			default:
				stringBuilder?.Append(str[i]);
				break;
			}
		}
		if (stringBuilder == null)
		{
			return str.Substring(start, end - start + 1);
		}
		return stringBuilder.ToString();
	}

	internal static string[] ReescapeWin32Strings(string[] array)
	{
		return array;
	}

	internal static string ReescapeWin32String(string str)
	{
		return str;
	}

	internal static bool IsCustomCultureId(int cultureId)
	{
		return false;
	}

	internal void GetNFIValues(NumberFormatInfo nfi)
	{
		if (!IsInvariantCulture)
		{
			fill_number_data(nfi, numberIndex);
		}
		nfi.percentDecimalDigits = nfi.numberDecimalDigits;
		nfi.percentDecimalSeparator = nfi.numberDecimalSeparator;
		nfi.percentGroupSizes = nfi.numberGroupSizes;
		nfi.percentGroupSeparator = nfi.numberGroupSeparator;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void fill_number_data(NumberFormatInfo nfi, int numberIndex);
}
