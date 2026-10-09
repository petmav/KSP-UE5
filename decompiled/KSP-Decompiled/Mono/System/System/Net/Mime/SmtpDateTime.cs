using System.Collections.Generic;
using System.Globalization;

namespace System.Net.Mime;

internal class SmtpDateTime
{
	internal const string unknownTimeZoneDefaultOffset = "-0000";

	internal const string utcDefaultTimeZoneOffset = "+0000";

	internal const int offsetLength = 5;

	internal const int maxMinuteValue = 59;

	internal const string dateFormatWithDayOfWeek = "ddd, dd MMM yyyy HH:mm:ss";

	internal const string dateFormatWithoutDayOfWeek = "dd MMM yyyy HH:mm:ss";

	internal const string dateFormatWithDayOfWeekAndNoSeconds = "ddd, dd MMM yyyy HH:mm";

	internal const string dateFormatWithoutDayOfWeekAndNoSeconds = "dd MMM yyyy HH:mm";

	internal static readonly string[] validDateTimeFormats = new string[4] { "ddd, dd MMM yyyy HH:mm:ss", "dd MMM yyyy HH:mm:ss", "ddd, dd MMM yyyy HH:mm", "dd MMM yyyy HH:mm" };

	internal static readonly char[] allowedWhiteSpaceChars = new char[2] { ' ', '\t' };

	internal static readonly IDictionary<string, TimeSpan> timeZoneOffsetLookup = InitializeShortHandLookups();

	internal static readonly long timeSpanMaxTicks = 3599400000000L;

	internal static readonly int offsetMaxValue = 9959;

	private readonly DateTime date;

	private readonly TimeSpan timeZone;

	private readonly bool unknownTimeZone;

	internal DateTime Date
	{
		get
		{
			if (unknownTimeZone)
			{
				return DateTime.SpecifyKind(date, DateTimeKind.Unspecified);
			}
			return new DateTimeOffset(date, timeZone).LocalDateTime;
		}
	}

	internal static IDictionary<string, TimeSpan> InitializeShortHandLookups()
	{
		return new Dictionary<string, TimeSpan>
		{
			{
				"UT",
				TimeSpan.Zero
			},
			{
				"GMT",
				TimeSpan.Zero
			},
			{
				"EDT",
				new TimeSpan(-4, 0, 0)
			},
			{
				"EST",
				new TimeSpan(-5, 0, 0)
			},
			{
				"CDT",
				new TimeSpan(-5, 0, 0)
			},
			{
				"CST",
				new TimeSpan(-6, 0, 0)
			},
			{
				"MDT",
				new TimeSpan(-6, 0, 0)
			},
			{
				"MST",
				new TimeSpan(-7, 0, 0)
			},
			{
				"PDT",
				new TimeSpan(-7, 0, 0)
			},
			{
				"PST",
				new TimeSpan(-8, 0, 0)
			}
		};
	}

	internal SmtpDateTime(DateTime value)
	{
		date = value;
		switch (value.Kind)
		{
		case DateTimeKind.Local:
		{
			TimeSpan utcOffset = TimeZoneInfo.Local.GetUtcOffset(value);
			timeZone = ValidateAndGetSanitizedTimeSpan(utcOffset);
			break;
		}
		case DateTimeKind.Unspecified:
			unknownTimeZone = true;
			break;
		case DateTimeKind.Utc:
			timeZone = TimeSpan.Zero;
			break;
		}
	}

	internal SmtpDateTime(string value)
	{
		date = ParseValue(value, out var timeZoneString);
		if (!TryParseTimeZoneString(timeZoneString, out timeZone))
		{
			unknownTimeZone = true;
		}
	}

	public override string ToString()
	{
		if (unknownTimeZone)
		{
			return string.Format("{0} {1}", FormatDate(date), "-0000");
		}
		return $"{FormatDate(date)} {TimeSpanToOffset(timeZone)}";
	}

	internal void ValidateAndGetTimeZoneOffsetValues(string offset, out bool positive, out int hours, out int minutes)
	{
		if (offset.Length != 5)
		{
			throw new FormatException(global::SR.GetString("The date is in an invalid format."));
		}
		positive = offset.StartsWith("+");
		if (!int.TryParse(offset.Substring(1, 2), NumberStyles.None, CultureInfo.InvariantCulture, out hours))
		{
			throw new FormatException(global::SR.GetString("The date is in an invalid format."));
		}
		if (!int.TryParse(offset.Substring(3, 2), NumberStyles.None, CultureInfo.InvariantCulture, out minutes))
		{
			throw new FormatException(global::SR.GetString("The date is in an invalid format."));
		}
		if (minutes > 59)
		{
			throw new FormatException(global::SR.GetString("The date is in an invalid format."));
		}
	}

	internal void ValidateTimeZoneShortHandValue(string value)
	{
		for (int i = 0; i < value.Length; i++)
		{
			if (!char.IsLetter(value, i))
			{
				throw new FormatException(global::SR.GetString("An invalid character was found in the mail header: '{0}'."));
			}
		}
	}

	internal string FormatDate(DateTime value)
	{
		return value.ToString("ddd, dd MMM yyyy HH:mm:ss", CultureInfo.InvariantCulture);
	}

	internal DateTime ParseValue(string data, out string timeZone)
	{
		if (string.IsNullOrEmpty(data))
		{
			throw new FormatException(global::SR.GetString("The date is in an invalid format."));
		}
		int num = data.IndexOf(':');
		if (num == -1)
		{
			throw new FormatException(global::SR.GetString("An invalid character was found in the mail header: '{0}'."));
		}
		int num2 = data.IndexOfAny(allowedWhiteSpaceChars, num);
		if (num2 == -1)
		{
			throw new FormatException(global::SR.GetString("An invalid character was found in the mail header: '{0}'."));
		}
		if (!DateTime.TryParseExact(data.Substring(0, num2).Trim(), validDateTimeFormats, CultureInfo.InvariantCulture, DateTimeStyles.AllowWhiteSpaces, out var result))
		{
			throw new FormatException(global::SR.GetString("The date is in an invalid format."));
		}
		string text = data.Substring(num2).Trim();
		int num3 = text.IndexOfAny(allowedWhiteSpaceChars);
		if (num3 != -1)
		{
			text = text.Substring(0, num3);
		}
		if (string.IsNullOrEmpty(text))
		{
			throw new FormatException(global::SR.GetString("The date is in an invalid format."));
		}
		timeZone = text;
		return result;
	}

	internal bool TryParseTimeZoneString(string timeZoneString, out TimeSpan timeZone)
	{
		timeZone = TimeSpan.Zero;
		if (timeZoneString == "-0000")
		{
			return false;
		}
		if (timeZoneString[0] == '+' || timeZoneString[0] == '-')
		{
			ValidateAndGetTimeZoneOffsetValues(timeZoneString, out var positive, out var hours, out var minutes);
			if (!positive)
			{
				if (hours != 0)
				{
					hours *= -1;
				}
				else if (minutes != 0)
				{
					minutes *= -1;
				}
			}
			timeZone = new TimeSpan(hours, minutes, 0);
			return true;
		}
		ValidateTimeZoneShortHandValue(timeZoneString);
		if (timeZoneOffsetLookup.ContainsKey(timeZoneString))
		{
			timeZone = timeZoneOffsetLookup[timeZoneString];
			return true;
		}
		return false;
	}

	internal TimeSpan ValidateAndGetSanitizedTimeSpan(TimeSpan span)
	{
		TimeSpan result = new TimeSpan(span.Days, span.Hours, span.Minutes, 0, 0);
		if (Math.Abs(result.Ticks) > timeSpanMaxTicks)
		{
			throw new FormatException(global::SR.GetString("The date is in an invalid format."));
		}
		return result;
	}

	internal string TimeSpanToOffset(TimeSpan span)
	{
		if (span.Ticks == 0L)
		{
			return "+0000";
		}
		uint num = (uint)Math.Abs(Math.Floor(span.TotalHours));
		uint num2 = (uint)Math.Abs(span.Minutes);
		string text = ((span.Ticks > 0) ? "+" : "-");
		if (num < 10)
		{
			text += "0";
		}
		text += num;
		if (num2 < 10)
		{
			text += "0";
		}
		return text + num2;
	}
}
