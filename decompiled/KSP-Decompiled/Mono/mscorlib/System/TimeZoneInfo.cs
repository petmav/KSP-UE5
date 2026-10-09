using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;
using System.Security.Permissions;
using System.Text;
using System.Threading;
using Microsoft.Win32;
using Unity;

namespace System;

[Serializable]
[TypeForwardedFrom("System.Core, Version=3.5.0.0, Culture=Neutral, PublicKeyToken=b77a5c561934e089")]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public sealed class TimeZoneInfo : IEquatable<TimeZoneInfo>, ISerializable, IDeserializationCallback
{
	[Serializable]
	[TypeForwardedFrom("System.Core, Version=3.5.0.0, Culture=Neutral, PublicKeyToken=b77a5c561934e089")]
	[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
	public sealed class AdjustmentRule : IEquatable<AdjustmentRule>, ISerializable, IDeserializationCallback
	{
		private DateTime m_dateStart;

		private DateTime m_dateEnd;

		private TimeSpan m_daylightDelta;

		private TransitionTime m_daylightTransitionStart;

		private TransitionTime m_daylightTransitionEnd;

		private TimeSpan m_baseUtcOffsetDelta;

		public DateTime DateStart => m_dateStart;

		public DateTime DateEnd => m_dateEnd;

		public TimeSpan DaylightDelta => m_daylightDelta;

		public TransitionTime DaylightTransitionStart => m_daylightTransitionStart;

		public TransitionTime DaylightTransitionEnd => m_daylightTransitionEnd;

		internal TimeSpan BaseUtcOffsetDelta => m_baseUtcOffsetDelta;

		internal bool HasDaylightSaving
		{
			get
			{
				if (!(DaylightDelta != TimeSpan.Zero) && !(DaylightTransitionStart.TimeOfDay != DateTime.MinValue))
				{
					return DaylightTransitionEnd.TimeOfDay != DateTime.MinValue.AddMilliseconds(1.0);
				}
				return true;
			}
		}

		public bool Equals(AdjustmentRule other)
		{
			if (other != null && m_dateStart == other.m_dateStart && m_dateEnd == other.m_dateEnd && m_daylightDelta == other.m_daylightDelta && m_baseUtcOffsetDelta == other.m_baseUtcOffsetDelta && m_daylightTransitionEnd.Equals(other.m_daylightTransitionEnd))
			{
				return m_daylightTransitionStart.Equals(other.m_daylightTransitionStart);
			}
			return false;
		}

		public override int GetHashCode()
		{
			return m_dateStart.GetHashCode();
		}

		private AdjustmentRule()
		{
		}

		public static AdjustmentRule CreateAdjustmentRule(DateTime dateStart, DateTime dateEnd, TimeSpan daylightDelta, TransitionTime daylightTransitionStart, TransitionTime daylightTransitionEnd)
		{
			ValidateAdjustmentRule(dateStart, dateEnd, daylightDelta, daylightTransitionStart, daylightTransitionEnd);
			return new AdjustmentRule
			{
				m_dateStart = dateStart,
				m_dateEnd = dateEnd,
				m_daylightDelta = daylightDelta,
				m_daylightTransitionStart = daylightTransitionStart,
				m_daylightTransitionEnd = daylightTransitionEnd,
				m_baseUtcOffsetDelta = TimeSpan.Zero
			};
		}

		internal static AdjustmentRule CreateAdjustmentRule(DateTime dateStart, DateTime dateEnd, TimeSpan daylightDelta, TransitionTime daylightTransitionStart, TransitionTime daylightTransitionEnd, TimeSpan baseUtcOffsetDelta)
		{
			AdjustmentRule adjustmentRule = CreateAdjustmentRule(dateStart, dateEnd, daylightDelta, daylightTransitionStart, daylightTransitionEnd);
			adjustmentRule.m_baseUtcOffsetDelta = baseUtcOffsetDelta;
			return adjustmentRule;
		}

		internal bool IsStartDateMarkerForBeginningOfYear()
		{
			if (DaylightTransitionStart.Month == 1 && DaylightTransitionStart.Day == 1 && DaylightTransitionStart.TimeOfDay.Hour == 0 && DaylightTransitionStart.TimeOfDay.Minute == 0 && DaylightTransitionStart.TimeOfDay.Second == 0)
			{
				return m_dateStart.Year == m_dateEnd.Year;
			}
			return false;
		}

		internal bool IsEndDateMarkerForEndOfYear()
		{
			if (DaylightTransitionEnd.Month == 1 && DaylightTransitionEnd.Day == 1 && DaylightTransitionEnd.TimeOfDay.Hour == 0 && DaylightTransitionEnd.TimeOfDay.Minute == 0 && DaylightTransitionEnd.TimeOfDay.Second == 0)
			{
				return m_dateStart.Year == m_dateEnd.Year;
			}
			return false;
		}

		private static void ValidateAdjustmentRule(DateTime dateStart, DateTime dateEnd, TimeSpan daylightDelta, TransitionTime daylightTransitionStart, TransitionTime daylightTransitionEnd)
		{
			if (dateStart.Kind != 0)
			{
				throw new ArgumentException(Environment.GetResourceString("The supplied DateTime must have the Kind property set to DateTimeKind.Unspecified."), "dateStart");
			}
			if (dateEnd.Kind != 0)
			{
				throw new ArgumentException(Environment.GetResourceString("The supplied DateTime must have the Kind property set to DateTimeKind.Unspecified."), "dateEnd");
			}
			if (daylightTransitionStart.Equals(daylightTransitionEnd))
			{
				throw new ArgumentException(Environment.GetResourceString("The DaylightTransitionStart property must not equal the DaylightTransitionEnd property."), "daylightTransitionEnd");
			}
			if (dateStart > dateEnd)
			{
				throw new ArgumentException(Environment.GetResourceString("The DateStart property must come before the DateEnd property."), "dateStart");
			}
			if (UtcOffsetOutOfRange(daylightDelta))
			{
				throw new ArgumentOutOfRangeException("daylightDelta", daylightDelta, Environment.GetResourceString("The TimeSpan parameter must be within plus or minus 14.0 hours."));
			}
			if (daylightDelta.Ticks % 600000000 != 0L)
			{
				throw new ArgumentException(Environment.GetResourceString("The TimeSpan parameter cannot be specified more precisely than whole minutes."), "daylightDelta");
			}
			if (dateStart.TimeOfDay != TimeSpan.Zero)
			{
				throw new ArgumentException(Environment.GetResourceString("The supplied DateTime includes a TimeOfDay setting.   This is not supported."), "dateStart");
			}
			if (dateEnd.TimeOfDay != TimeSpan.Zero)
			{
				throw new ArgumentException(Environment.GetResourceString("The supplied DateTime includes a TimeOfDay setting.   This is not supported."), "dateEnd");
			}
		}

		void IDeserializationCallback.OnDeserialization(object sender)
		{
			try
			{
				ValidateAdjustmentRule(m_dateStart, m_dateEnd, m_daylightDelta, m_daylightTransitionStart, m_daylightTransitionEnd);
			}
			catch (ArgumentException innerException)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."), innerException);
			}
		}

		[SecurityCritical]
		void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
		{
			if (info == null)
			{
				throw new ArgumentNullException("info");
			}
			info.AddValue("DateStart", m_dateStart);
			info.AddValue("DateEnd", m_dateEnd);
			info.AddValue("DaylightDelta", m_daylightDelta);
			info.AddValue("DaylightTransitionStart", m_daylightTransitionStart);
			info.AddValue("DaylightTransitionEnd", m_daylightTransitionEnd);
			info.AddValue("BaseUtcOffsetDelta", m_baseUtcOffsetDelta);
		}

		private AdjustmentRule(SerializationInfo info, StreamingContext context)
		{
			if (info == null)
			{
				throw new ArgumentNullException("info");
			}
			m_dateStart = (DateTime)info.GetValue("DateStart", typeof(DateTime));
			m_dateEnd = (DateTime)info.GetValue("DateEnd", typeof(DateTime));
			m_daylightDelta = (TimeSpan)info.GetValue("DaylightDelta", typeof(TimeSpan));
			m_daylightTransitionStart = (TransitionTime)info.GetValue("DaylightTransitionStart", typeof(TransitionTime));
			m_daylightTransitionEnd = (TransitionTime)info.GetValue("DaylightTransitionEnd", typeof(TransitionTime));
			object valueNoThrow = info.GetValueNoThrow("BaseUtcOffsetDelta", typeof(TimeSpan));
			if (valueNoThrow != null)
			{
				m_baseUtcOffsetDelta = (TimeSpan)valueNoThrow;
			}
		}
	}

	[Serializable]
	[TypeForwardedFrom("System.Core, Version=3.5.0.0, Culture=Neutral, PublicKeyToken=b77a5c561934e089")]
	[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
	public struct TransitionTime : IEquatable<TransitionTime>, ISerializable, IDeserializationCallback
	{
		private DateTime m_timeOfDay;

		private byte m_month;

		private byte m_week;

		private byte m_day;

		private DayOfWeek m_dayOfWeek;

		private bool m_isFixedDateRule;

		public DateTime TimeOfDay => m_timeOfDay;

		public int Month => m_month;

		public int Week => m_week;

		public int Day => m_day;

		public DayOfWeek DayOfWeek => m_dayOfWeek;

		public bool IsFixedDateRule => m_isFixedDateRule;

		public override bool Equals(object obj)
		{
			if (obj is TransitionTime)
			{
				return Equals((TransitionTime)obj);
			}
			return false;
		}

		public static bool operator ==(TransitionTime t1, TransitionTime t2)
		{
			return t1.Equals(t2);
		}

		public static bool operator !=(TransitionTime t1, TransitionTime t2)
		{
			return !t1.Equals(t2);
		}

		public bool Equals(TransitionTime other)
		{
			bool flag = m_isFixedDateRule == other.m_isFixedDateRule && m_timeOfDay == other.m_timeOfDay && m_month == other.m_month;
			if (flag)
			{
				flag = ((!other.m_isFixedDateRule) ? (m_week == other.m_week && m_dayOfWeek == other.m_dayOfWeek) : (m_day == other.m_day));
			}
			return flag;
		}

		public override int GetHashCode()
		{
			return m_month ^ (m_week << 8);
		}

		public static TransitionTime CreateFixedDateRule(DateTime timeOfDay, int month, int day)
		{
			return CreateTransitionTime(timeOfDay, month, 1, day, DayOfWeek.Sunday, isFixedDateRule: true);
		}

		public static TransitionTime CreateFloatingDateRule(DateTime timeOfDay, int month, int week, DayOfWeek dayOfWeek)
		{
			return CreateTransitionTime(timeOfDay, month, week, 1, dayOfWeek, isFixedDateRule: false);
		}

		private static TransitionTime CreateTransitionTime(DateTime timeOfDay, int month, int week, int day, DayOfWeek dayOfWeek, bool isFixedDateRule)
		{
			ValidateTransitionTime(timeOfDay, month, week, day, dayOfWeek);
			TransitionTime result = default(TransitionTime);
			result.m_isFixedDateRule = isFixedDateRule;
			result.m_timeOfDay = timeOfDay;
			result.m_dayOfWeek = dayOfWeek;
			result.m_day = (byte)day;
			result.m_week = (byte)week;
			result.m_month = (byte)month;
			return result;
		}

		private static void ValidateTransitionTime(DateTime timeOfDay, int month, int week, int day, DayOfWeek dayOfWeek)
		{
			if (timeOfDay.Kind != 0)
			{
				throw new ArgumentException(Environment.GetResourceString("The supplied DateTime must have the Kind property set to DateTimeKind.Unspecified."), "timeOfDay");
			}
			if (month < 1 || month > 12)
			{
				throw new ArgumentOutOfRangeException("month", Environment.GetResourceString("The Month parameter must be in the range 1 through 12."));
			}
			if (day < 1 || day > 31)
			{
				throw new ArgumentOutOfRangeException("day", Environment.GetResourceString("The Day parameter must be in the range 1 through 31."));
			}
			if (week < 1 || week > 5)
			{
				throw new ArgumentOutOfRangeException("week", Environment.GetResourceString("The Week parameter must be in the range 1 through 5."));
			}
			if (dayOfWeek < DayOfWeek.Sunday || dayOfWeek > DayOfWeek.Saturday)
			{
				throw new ArgumentOutOfRangeException("dayOfWeek", Environment.GetResourceString("The DayOfWeek enumeration must be in the range 0 through 6."));
			}
			if (timeOfDay.Year != 1 || timeOfDay.Month != 1 || timeOfDay.Day != 1 || timeOfDay.Ticks % 10000 != 0L)
			{
				throw new ArgumentException(Environment.GetResourceString("The supplied DateTime must have the Year, Month, and Day properties set to 1.  The time cannot be specified more precisely than whole milliseconds."), "timeOfDay");
			}
		}

		void IDeserializationCallback.OnDeserialization(object sender)
		{
			try
			{
				ValidateTransitionTime(m_timeOfDay, m_month, m_week, m_day, m_dayOfWeek);
			}
			catch (ArgumentException innerException)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."), innerException);
			}
		}

		[SecurityCritical]
		void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
		{
			if (info == null)
			{
				throw new ArgumentNullException("info");
			}
			info.AddValue("TimeOfDay", m_timeOfDay);
			info.AddValue("Month", m_month);
			info.AddValue("Week", m_week);
			info.AddValue("Day", m_day);
			info.AddValue("DayOfWeek", m_dayOfWeek);
			info.AddValue("IsFixedDateRule", m_isFixedDateRule);
		}

		private TransitionTime(SerializationInfo info, StreamingContext context)
		{
			if (info == null)
			{
				throw new ArgumentNullException("info");
			}
			m_timeOfDay = (DateTime)info.GetValue("TimeOfDay", typeof(DateTime));
			m_month = (byte)info.GetValue("Month", typeof(byte));
			m_week = (byte)info.GetValue("Week", typeof(byte));
			m_day = (byte)info.GetValue("Day", typeof(byte));
			m_dayOfWeek = (DayOfWeek)info.GetValue("DayOfWeek", typeof(DayOfWeek));
			m_isFixedDateRule = (bool)info.GetValue("IsFixedDateRule", typeof(bool));
		}
	}

	private sealed class StringSerializer
	{
		private enum State
		{
			Escaped,
			NotEscaped,
			StartOfToken,
			EndOfLine
		}

		private string m_serializedText;

		private int m_currentTokenStartIndex;

		private State m_state;

		private const int initialCapacityForString = 64;

		private const char esc = '\\';

		private const char sep = ';';

		private const char lhs = '[';

		private const char rhs = ']';

		private const string escString = "\\";

		private const string sepString = ";";

		private const string lhsString = "[";

		private const string rhsString = "]";

		private const string escapedEsc = "\\\\";

		private const string escapedSep = "\\;";

		private const string escapedLhs = "\\[";

		private const string escapedRhs = "\\]";

		private const string dateTimeFormat = "MM:dd:yyyy";

		private const string timeOfDayFormat = "HH:mm:ss.FFF";

		public static string GetSerializedString(TimeZoneInfo zone)
		{
			StringBuilder stringBuilder = StringBuilderCache.Acquire();
			stringBuilder.Append(SerializeSubstitute(zone.Id));
			stringBuilder.Append(';');
			stringBuilder.Append(SerializeSubstitute(zone.BaseUtcOffset.TotalMinutes.ToString(CultureInfo.InvariantCulture)));
			stringBuilder.Append(';');
			stringBuilder.Append(SerializeSubstitute(zone.DisplayName));
			stringBuilder.Append(';');
			stringBuilder.Append(SerializeSubstitute(zone.StandardName));
			stringBuilder.Append(';');
			stringBuilder.Append(SerializeSubstitute(zone.DaylightName));
			stringBuilder.Append(';');
			AdjustmentRule[] adjustmentRules = zone.GetAdjustmentRules();
			if (adjustmentRules != null && adjustmentRules.Length != 0)
			{
				foreach (AdjustmentRule adjustmentRule in adjustmentRules)
				{
					stringBuilder.Append('[');
					stringBuilder.Append(SerializeSubstitute(adjustmentRule.DateStart.ToString("MM:dd:yyyy", DateTimeFormatInfo.InvariantInfo)));
					stringBuilder.Append(';');
					stringBuilder.Append(SerializeSubstitute(adjustmentRule.DateEnd.ToString("MM:dd:yyyy", DateTimeFormatInfo.InvariantInfo)));
					stringBuilder.Append(';');
					stringBuilder.Append(SerializeSubstitute(adjustmentRule.DaylightDelta.TotalMinutes.ToString(CultureInfo.InvariantCulture)));
					stringBuilder.Append(';');
					SerializeTransitionTime(adjustmentRule.DaylightTransitionStart, stringBuilder);
					stringBuilder.Append(';');
					SerializeTransitionTime(adjustmentRule.DaylightTransitionEnd, stringBuilder);
					stringBuilder.Append(';');
					if (adjustmentRule.BaseUtcOffsetDelta != TimeSpan.Zero)
					{
						stringBuilder.Append(SerializeSubstitute(adjustmentRule.BaseUtcOffsetDelta.TotalMinutes.ToString(CultureInfo.InvariantCulture)));
						stringBuilder.Append(';');
					}
					stringBuilder.Append(']');
				}
			}
			stringBuilder.Append(';');
			return StringBuilderCache.GetStringAndRelease(stringBuilder);
		}

		public static TimeZoneInfo GetDeserializedTimeZoneInfo(string source)
		{
			StringSerializer stringSerializer = new StringSerializer(source);
			string nextStringValue = stringSerializer.GetNextStringValue(canEndWithoutSeparator: false);
			TimeSpan nextTimeSpanValue = stringSerializer.GetNextTimeSpanValue(canEndWithoutSeparator: false);
			string nextStringValue2 = stringSerializer.GetNextStringValue(canEndWithoutSeparator: false);
			string nextStringValue3 = stringSerializer.GetNextStringValue(canEndWithoutSeparator: false);
			string nextStringValue4 = stringSerializer.GetNextStringValue(canEndWithoutSeparator: false);
			AdjustmentRule[] nextAdjustmentRuleArrayValue = stringSerializer.GetNextAdjustmentRuleArrayValue(canEndWithoutSeparator: false);
			try
			{
				return CreateCustomTimeZone(nextStringValue, nextTimeSpanValue, nextStringValue2, nextStringValue3, nextStringValue4, nextAdjustmentRuleArrayValue);
			}
			catch (ArgumentException innerException)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."), innerException);
			}
			catch (InvalidTimeZoneException innerException2)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."), innerException2);
			}
		}

		private StringSerializer(string str)
		{
			m_serializedText = str;
			m_state = State.StartOfToken;
		}

		private static string SerializeSubstitute(string text)
		{
			text = text.Replace("\\", "\\\\");
			text = text.Replace("[", "\\[");
			text = text.Replace("]", "\\]");
			return text.Replace(";", "\\;");
		}

		private static void SerializeTransitionTime(TransitionTime time, StringBuilder serializedText)
		{
			serializedText.Append('[');
			serializedText.Append((time.IsFixedDateRule ? 1 : 0).ToString(CultureInfo.InvariantCulture));
			serializedText.Append(';');
			if (time.IsFixedDateRule)
			{
				serializedText.Append(SerializeSubstitute(time.TimeOfDay.ToString("HH:mm:ss.FFF", DateTimeFormatInfo.InvariantInfo)));
				serializedText.Append(';');
				serializedText.Append(SerializeSubstitute(time.Month.ToString(CultureInfo.InvariantCulture)));
				serializedText.Append(';');
				serializedText.Append(SerializeSubstitute(time.Day.ToString(CultureInfo.InvariantCulture)));
				serializedText.Append(';');
			}
			else
			{
				serializedText.Append(SerializeSubstitute(time.TimeOfDay.ToString("HH:mm:ss.FFF", DateTimeFormatInfo.InvariantInfo)));
				serializedText.Append(';');
				serializedText.Append(SerializeSubstitute(time.Month.ToString(CultureInfo.InvariantCulture)));
				serializedText.Append(';');
				serializedText.Append(SerializeSubstitute(time.Week.ToString(CultureInfo.InvariantCulture)));
				serializedText.Append(';');
				serializedText.Append(SerializeSubstitute(((int)time.DayOfWeek).ToString(CultureInfo.InvariantCulture)));
				serializedText.Append(';');
			}
			serializedText.Append(']');
		}

		private static void VerifyIsEscapableCharacter(char c)
		{
			if (c != '\\' && c != ';' && c != '[' && c != ']')
			{
				throw new SerializationException(Environment.GetResourceString("The serialized data contained an invalid escape sequence '\\\\{0}'.", c));
			}
		}

		private void SkipVersionNextDataFields(int depth)
		{
			if (m_currentTokenStartIndex < 0 || m_currentTokenStartIndex >= m_serializedText.Length)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			State state = State.NotEscaped;
			for (int i = m_currentTokenStartIndex; i < m_serializedText.Length; i++)
			{
				switch (state)
				{
				case State.Escaped:
					VerifyIsEscapableCharacter(m_serializedText[i]);
					state = State.NotEscaped;
					break;
				case State.NotEscaped:
					switch (m_serializedText[i])
					{
					case '\\':
						state = State.Escaped;
						break;
					case '[':
						depth++;
						break;
					case ']':
						depth--;
						if (depth == 0)
						{
							m_currentTokenStartIndex = i + 1;
							if (m_currentTokenStartIndex >= m_serializedText.Length)
							{
								m_state = State.EndOfLine;
							}
							else
							{
								m_state = State.StartOfToken;
							}
							return;
						}
						break;
					case '\0':
						throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
					}
					break;
				}
			}
			throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
		}

		private string GetNextStringValue(bool canEndWithoutSeparator)
		{
			if (m_state == State.EndOfLine)
			{
				if (canEndWithoutSeparator)
				{
					return null;
				}
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			if (m_currentTokenStartIndex < 0 || m_currentTokenStartIndex >= m_serializedText.Length)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			State state = State.NotEscaped;
			StringBuilder stringBuilder = StringBuilderCache.Acquire(64);
			for (int i = m_currentTokenStartIndex; i < m_serializedText.Length; i++)
			{
				switch (state)
				{
				case State.Escaped:
					VerifyIsEscapableCharacter(m_serializedText[i]);
					stringBuilder.Append(m_serializedText[i]);
					state = State.NotEscaped;
					break;
				case State.NotEscaped:
					switch (m_serializedText[i])
					{
					case '\\':
						state = State.Escaped;
						break;
					case '[':
						throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
					case ']':
						if (canEndWithoutSeparator)
						{
							m_currentTokenStartIndex = i;
							m_state = State.StartOfToken;
							return stringBuilder.ToString();
						}
						throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
					case ';':
						m_currentTokenStartIndex = i + 1;
						if (m_currentTokenStartIndex >= m_serializedText.Length)
						{
							m_state = State.EndOfLine;
						}
						else
						{
							m_state = State.StartOfToken;
						}
						return StringBuilderCache.GetStringAndRelease(stringBuilder);
					case '\0':
						throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
					default:
						stringBuilder.Append(m_serializedText[i]);
						break;
					}
					break;
				}
			}
			if (state == State.Escaped)
			{
				throw new SerializationException(Environment.GetResourceString("The serialized data contained an invalid escape sequence '\\\\{0}'.", string.Empty));
			}
			if (!canEndWithoutSeparator)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			m_currentTokenStartIndex = m_serializedText.Length;
			m_state = State.EndOfLine;
			return StringBuilderCache.GetStringAndRelease(stringBuilder);
		}

		private DateTime GetNextDateTimeValue(bool canEndWithoutSeparator, string format)
		{
			if (!DateTime.TryParseExact(GetNextStringValue(canEndWithoutSeparator), format, DateTimeFormatInfo.InvariantInfo, DateTimeStyles.None, out var result))
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			return result;
		}

		private TimeSpan GetNextTimeSpanValue(bool canEndWithoutSeparator)
		{
			int nextInt32Value = GetNextInt32Value(canEndWithoutSeparator);
			try
			{
				return new TimeSpan(0, nextInt32Value, 0);
			}
			catch (ArgumentOutOfRangeException innerException)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."), innerException);
			}
		}

		private int GetNextInt32Value(bool canEndWithoutSeparator)
		{
			if (!int.TryParse(GetNextStringValue(canEndWithoutSeparator), NumberStyles.AllowLeadingSign, CultureInfo.InvariantCulture, out var result))
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			return result;
		}

		private AdjustmentRule[] GetNextAdjustmentRuleArrayValue(bool canEndWithoutSeparator)
		{
			List<AdjustmentRule> list = new List<AdjustmentRule>(1);
			int num = 0;
			for (AdjustmentRule nextAdjustmentRuleValue = GetNextAdjustmentRuleValue(canEndWithoutSeparator: true); nextAdjustmentRuleValue != null; nextAdjustmentRuleValue = GetNextAdjustmentRuleValue(canEndWithoutSeparator: true))
			{
				list.Add(nextAdjustmentRuleValue);
				num++;
			}
			if (!canEndWithoutSeparator)
			{
				if (m_state == State.EndOfLine)
				{
					throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
				}
				if (m_currentTokenStartIndex < 0 || m_currentTokenStartIndex >= m_serializedText.Length)
				{
					throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
				}
			}
			if (num == 0)
			{
				return null;
			}
			return list.ToArray();
		}

		private AdjustmentRule GetNextAdjustmentRuleValue(bool canEndWithoutSeparator)
		{
			if (m_state == State.EndOfLine)
			{
				if (canEndWithoutSeparator)
				{
					return null;
				}
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			if (m_currentTokenStartIndex < 0 || m_currentTokenStartIndex >= m_serializedText.Length)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			if (m_serializedText[m_currentTokenStartIndex] == ';')
			{
				return null;
			}
			if (m_serializedText[m_currentTokenStartIndex] != '[')
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			m_currentTokenStartIndex++;
			DateTime nextDateTimeValue = GetNextDateTimeValue(canEndWithoutSeparator: false, "MM:dd:yyyy");
			DateTime nextDateTimeValue2 = GetNextDateTimeValue(canEndWithoutSeparator: false, "MM:dd:yyyy");
			TimeSpan nextTimeSpanValue = GetNextTimeSpanValue(canEndWithoutSeparator: false);
			TransitionTime nextTransitionTimeValue = GetNextTransitionTimeValue(canEndWithoutSeparator: false);
			TransitionTime nextTransitionTimeValue2 = GetNextTransitionTimeValue(canEndWithoutSeparator: false);
			TimeSpan baseUtcOffsetDelta = TimeSpan.Zero;
			if (m_state == State.EndOfLine || m_currentTokenStartIndex >= m_serializedText.Length)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			if ((m_serializedText[m_currentTokenStartIndex] >= '0' && m_serializedText[m_currentTokenStartIndex] <= '9') || m_serializedText[m_currentTokenStartIndex] == '-' || m_serializedText[m_currentTokenStartIndex] == '+')
			{
				baseUtcOffsetDelta = GetNextTimeSpanValue(canEndWithoutSeparator: false);
			}
			if (m_state == State.EndOfLine || m_currentTokenStartIndex >= m_serializedText.Length)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			if (m_serializedText[m_currentTokenStartIndex] != ']')
			{
				SkipVersionNextDataFields(1);
			}
			else
			{
				m_currentTokenStartIndex++;
			}
			AdjustmentRule result;
			try
			{
				result = AdjustmentRule.CreateAdjustmentRule(nextDateTimeValue, nextDateTimeValue2, nextTimeSpanValue, nextTransitionTimeValue, nextTransitionTimeValue2, baseUtcOffsetDelta);
			}
			catch (ArgumentException innerException)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."), innerException);
			}
			if (m_currentTokenStartIndex >= m_serializedText.Length)
			{
				m_state = State.EndOfLine;
			}
			else
			{
				m_state = State.StartOfToken;
			}
			return result;
		}

		private TransitionTime GetNextTransitionTimeValue(bool canEndWithoutSeparator)
		{
			if (m_state == State.EndOfLine || (m_currentTokenStartIndex < m_serializedText.Length && m_serializedText[m_currentTokenStartIndex] == ']'))
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			if (m_currentTokenStartIndex < 0 || m_currentTokenStartIndex >= m_serializedText.Length)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			if (m_serializedText[m_currentTokenStartIndex] != '[')
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			m_currentTokenStartIndex++;
			int nextInt32Value = GetNextInt32Value(canEndWithoutSeparator: false);
			if (nextInt32Value != 0 && nextInt32Value != 1)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			DateTime nextDateTimeValue = GetNextDateTimeValue(canEndWithoutSeparator: false, "HH:mm:ss.FFF");
			nextDateTimeValue = new DateTime(1, 1, 1, nextDateTimeValue.Hour, nextDateTimeValue.Minute, nextDateTimeValue.Second, nextDateTimeValue.Millisecond);
			int nextInt32Value2 = GetNextInt32Value(canEndWithoutSeparator: false);
			TransitionTime result;
			if (nextInt32Value == 1)
			{
				int nextInt32Value3 = GetNextInt32Value(canEndWithoutSeparator: false);
				try
				{
					result = TransitionTime.CreateFixedDateRule(nextDateTimeValue, nextInt32Value2, nextInt32Value3);
				}
				catch (ArgumentException innerException)
				{
					throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."), innerException);
				}
			}
			else
			{
				int nextInt32Value4 = GetNextInt32Value(canEndWithoutSeparator: false);
				int nextInt32Value5 = GetNextInt32Value(canEndWithoutSeparator: false);
				try
				{
					result = TransitionTime.CreateFloatingDateRule(nextDateTimeValue, nextInt32Value2, nextInt32Value4, (DayOfWeek)nextInt32Value5);
				}
				catch (ArgumentException innerException2)
				{
					throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."), innerException2);
				}
			}
			if (m_state == State.EndOfLine || m_currentTokenStartIndex >= m_serializedText.Length)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			if (m_serializedText[m_currentTokenStartIndex] != ']')
			{
				SkipVersionNextDataFields(1);
			}
			else
			{
				m_currentTokenStartIndex++;
			}
			bool flag = false;
			if (m_currentTokenStartIndex < m_serializedText.Length && m_serializedText[m_currentTokenStartIndex] == ';')
			{
				m_currentTokenStartIndex++;
				flag = true;
			}
			if (!flag && !canEndWithoutSeparator)
			{
				throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."));
			}
			if (m_currentTokenStartIndex >= m_serializedText.Length)
			{
				m_state = State.EndOfLine;
			}
			else
			{
				m_state = State.StartOfToken;
			}
			return result;
		}
	}

	private class TimeZoneInfoComparer : IComparer<TimeZoneInfo>
	{
		int IComparer<TimeZoneInfo>.Compare(TimeZoneInfo x, TimeZoneInfo y)
		{
			int num = x.BaseUtcOffset.CompareTo(y.BaseUtcOffset);
			if (num != 0)
			{
				return num;
			}
			return string.Compare(x.DisplayName, y.DisplayName, StringComparison.Ordinal);
		}
	}

	private enum TimeZoneData
	{
		DaylightSavingFirstTransitionIdx,
		DaylightSavingSecondTransitionIdx,
		UtcOffsetIdx,
		AdditionalDaylightOffsetIdx
	}

	private enum TimeZoneNames
	{
		StandardNameIdx,
		DaylightNameIdx
	}

	internal struct SYSTEMTIME
	{
		internal ushort wYear;

		internal ushort wMonth;

		internal ushort wDayOfWeek;

		internal ushort wDay;

		internal ushort wHour;

		internal ushort wMinute;

		internal ushort wSecond;

		internal ushort wMilliseconds;
	}

	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
	internal struct TIME_ZONE_INFORMATION
	{
		internal int Bias;

		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
		internal string StandardName;

		internal SYSTEMTIME StandardDate;

		internal int StandardBias;

		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
		internal string DaylightName;

		internal SYSTEMTIME DaylightDate;

		internal int DaylightBias;
	}

	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
	internal struct DYNAMIC_TIME_ZONE_INFORMATION
	{
		internal TIME_ZONE_INFORMATION TZI;

		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
		internal string TimeZoneKeyName;

		internal byte DynamicDaylightTimeDisabled;
	}

	internal const uint TIME_ZONE_ID_INVALID = uint.MaxValue;

	internal const uint ERROR_NO_MORE_ITEMS = 259u;

	internal const uint ERROR_SUCCESS = 0u;

	private TimeSpan baseUtcOffset;

	private string daylightDisplayName;

	private string displayName;

	private string id;

	private static TimeZoneInfo local;

	private List<KeyValuePair<DateTime, TimeType>> transitions;

	private static bool readlinkNotFound;

	private string standardDisplayName;

	private bool supportsDaylightSavingTime;

	private static TimeZoneInfo utc;

	private static string timeZoneDirectory;

	private AdjustmentRule[] adjustmentRules;

	private static RegistryKey timeZoneKey;

	private static RegistryKey localZoneKey;

	private static ReadOnlyCollection<TimeZoneInfo> systemTimeZones;

	private const int BUFFER_SIZE = 16384;

	public TimeSpan BaseUtcOffset => baseUtcOffset;

	public string DaylightName
	{
		get
		{
			if (!supportsDaylightSavingTime)
			{
				return string.Empty;
			}
			return daylightDisplayName;
		}
	}

	public string DisplayName => displayName;

	public string Id => id;

	public static TimeZoneInfo Local
	{
		get
		{
			TimeZoneInfo timeZoneInfo = local;
			if (timeZoneInfo == null)
			{
				timeZoneInfo = CreateLocal();
				if (timeZoneInfo == null)
				{
					throw new TimeZoneNotFoundException();
				}
				if (Interlocked.CompareExchange(ref local, timeZoneInfo, null) != null)
				{
					timeZoneInfo = local;
				}
			}
			return timeZoneInfo;
		}
	}

	public string StandardName => standardDisplayName;

	public bool SupportsDaylightSavingTime => supportsDaylightSavingTime;

	public static TimeZoneInfo Utc
	{
		get
		{
			if (utc == null)
			{
				utc = CreateCustomTimeZone("UTC", new TimeSpan(0L), "UTC", "UTC");
			}
			return utc;
		}
	}

	private static string TimeZoneDirectory
	{
		get
		{
			if (timeZoneDirectory == null)
			{
				timeZoneDirectory = "/usr/share/zoneinfo";
			}
			return timeZoneDirectory;
		}
		set
		{
			ClearCachedData();
			timeZoneDirectory = value;
		}
	}

	private static bool IsWindows
	{
		get
		{
			int platform = (int)Environment.OSVersion.Platform;
			if (platform != 4 && platform != 6)
			{
				return platform != 128;
			}
			return false;
		}
	}

	private static RegistryKey TimeZoneKey
	{
		get
		{
			if (timeZoneKey != null)
			{
				return timeZoneKey;
			}
			if (!IsWindows)
			{
				return null;
			}
			try
			{
				return timeZoneKey = Registry.LocalMachine.OpenSubKey("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones", writable: false);
			}
			catch
			{
				return null;
			}
		}
	}

	private static RegistryKey LocalZoneKey
	{
		get
		{
			if (localZoneKey != null)
			{
				return localZoneKey;
			}
			if (!IsWindows)
			{
				return null;
			}
			try
			{
				return localZoneKey = Registry.LocalMachine.OpenSubKey("SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation", writable: false);
			}
			catch
			{
				return null;
			}
		}
	}

	internal static bool UtcOffsetOutOfRange(TimeSpan offset)
	{
		if (!(offset.TotalHours < -14.0))
		{
			return offset.TotalHours > 14.0;
		}
		return true;
	}

	private static void ValidateTimeZoneInfo(string id, TimeSpan baseUtcOffset, AdjustmentRule[] adjustmentRules, out bool adjustmentRulesSupportDst)
	{
		if (id == null)
		{
			throw new ArgumentNullException("id");
		}
		if (id.Length == 0)
		{
			throw new ArgumentException(Environment.GetResourceString("The specified ID parameter '{0}' is not supported.", id), "id");
		}
		if (UtcOffsetOutOfRange(baseUtcOffset))
		{
			throw new ArgumentOutOfRangeException("baseUtcOffset", Environment.GetResourceString("The TimeSpan parameter must be within plus or minus 14.0 hours."));
		}
		if (baseUtcOffset.Ticks % 600000000 != 0L)
		{
			throw new ArgumentException(Environment.GetResourceString("The TimeSpan parameter cannot be specified more precisely than whole minutes."), "baseUtcOffset");
		}
		adjustmentRulesSupportDst = false;
		if (adjustmentRules == null || adjustmentRules.Length == 0)
		{
			return;
		}
		adjustmentRulesSupportDst = true;
		AdjustmentRule adjustmentRule = null;
		AdjustmentRule adjustmentRule2 = null;
		for (int i = 0; i < adjustmentRules.Length; i++)
		{
			adjustmentRule = adjustmentRule2;
			adjustmentRule2 = adjustmentRules[i];
			if (adjustmentRule2 == null)
			{
				throw new InvalidTimeZoneException(Environment.GetResourceString("The AdjustmentRule array cannot contain null elements."));
			}
			if (UtcOffsetOutOfRange(baseUtcOffset + adjustmentRule2.DaylightDelta))
			{
				throw new InvalidTimeZoneException(Environment.GetResourceString("The sum of the BaseUtcOffset and DaylightDelta properties must within plus or minus 14.0 hours."));
			}
			if (adjustmentRule != null && adjustmentRule2.DateStart <= adjustmentRule.DateEnd)
			{
				throw new InvalidTimeZoneException(Environment.GetResourceString("The elements of the AdjustmentRule array must be in chronological order and must not overlap."));
			}
		}
	}

	public static TimeZoneInfo FromSerializedString(string source)
	{
		StringBuilder input = new StringBuilder(source);
		string text = DeserializeString(ref input);
		int num = DeserializeInt(ref input);
		string text2 = DeserializeString(ref input);
		string text3 = DeserializeString(ref input);
		string text4 = DeserializeString(ref input);
		List<AdjustmentRule> list = null;
		while (input[0] != ';')
		{
			if (list == null)
			{
				list = new List<AdjustmentRule>();
			}
			list.Add(DeserializeAdjustmentRule(ref input));
		}
		TimeSpan timeSpan = TimeSpan.FromMinutes(num);
		return CreateCustomTimeZone(text, timeSpan, text2, text3, text4, list?.ToArray());
	}

	public string ToSerializedString()
	{
		StringBuilder stringBuilder = new StringBuilder();
		string unescaped = (string.IsNullOrEmpty(DaylightName) ? StandardName : DaylightName);
		stringBuilder.AppendFormat("{0};{1};{2};{3};{4};", EscapeForSerialization(Id), (int)BaseUtcOffset.TotalMinutes, EscapeForSerialization(DisplayName), EscapeForSerialization(StandardName), EscapeForSerialization(unescaped));
		if (SupportsDaylightSavingTime)
		{
			AdjustmentRule[] array = GetAdjustmentRules();
			foreach (AdjustmentRule obj in array)
			{
				string text = obj.DateStart.ToString("MM:dd:yyyy", CultureInfo.InvariantCulture);
				string text2 = obj.DateEnd.ToString("MM:dd:yyyy", CultureInfo.InvariantCulture);
				int num = (int)obj.DaylightDelta.TotalMinutes;
				string text3 = SerializeTransitionTime(obj.DaylightTransitionStart);
				string text4 = SerializeTransitionTime(obj.DaylightTransitionEnd);
				stringBuilder.AppendFormat("[{0};{1};{2};{3};{4};]", text, text2, num, text3, text4);
			}
		}
		stringBuilder.Append(";");
		return stringBuilder.ToString();
	}

	private static AdjustmentRule DeserializeAdjustmentRule(ref StringBuilder input)
	{
		if (input[0] != '[')
		{
			throw new SerializationException();
		}
		input.Remove(0, 1);
		DateTime dateStart = DeserializeDate(ref input);
		DateTime dateEnd = DeserializeDate(ref input);
		int num = DeserializeInt(ref input);
		TransitionTime daylightTransitionStart = DeserializeTransitionTime(ref input);
		TransitionTime daylightTransitionEnd = DeserializeTransitionTime(ref input);
		input.Remove(0, 1);
		TimeSpan daylightDelta = TimeSpan.FromMinutes(num);
		return AdjustmentRule.CreateAdjustmentRule(dateStart, dateEnd, daylightDelta, daylightTransitionStart, daylightTransitionEnd);
	}

	private static TransitionTime DeserializeTransitionTime(ref StringBuilder input)
	{
		if (input[0] != '[' || (input[1] != '0' && input[1] != '1') || input[2] != ';')
		{
			throw new SerializationException();
		}
		char num = input[1];
		input.Remove(0, 3);
		DateTime timeOfDay = DeserializeTime(ref input);
		int month = DeserializeInt(ref input);
		if (num == '0')
		{
			int week = DeserializeInt(ref input);
			int dayOfWeek = DeserializeInt(ref input);
			input.Remove(0, 2);
			return TransitionTime.CreateFloatingDateRule(timeOfDay, month, week, (DayOfWeek)dayOfWeek);
		}
		int day = DeserializeInt(ref input);
		input.Remove(0, 2);
		return TransitionTime.CreateFixedDateRule(timeOfDay, month, day);
	}

	private static string DeserializeString(ref StringBuilder input)
	{
		StringBuilder stringBuilder = new StringBuilder();
		bool flag = false;
		int i;
		for (i = 0; i < input.Length; i++)
		{
			char c = input[i];
			if (flag)
			{
				flag = false;
				stringBuilder.Append(c);
				continue;
			}
			switch (c)
			{
			case '\\':
				flag = true;
				continue;
			default:
				stringBuilder.Append(c);
				continue;
			case ';':
				break;
			}
			break;
		}
		input.Remove(0, i + 1);
		return stringBuilder.ToString();
	}

	private static int DeserializeInt(ref StringBuilder input)
	{
		int num = 0;
		while (num++ < input.Length && input[num] != ';')
		{
		}
		if (!int.TryParse(input.ToString(0, num), NumberStyles.Integer, CultureInfo.InvariantCulture, out var result))
		{
			throw new SerializationException();
		}
		input.Remove(0, num + 1);
		return result;
	}

	private static DateTime DeserializeDate(ref StringBuilder input)
	{
		char[] array = new char[11];
		input.CopyTo(0, array, 0, array.Length);
		if (!DateTime.TryParseExact(new string(array), "MM:dd:yyyy;", CultureInfo.InvariantCulture, DateTimeStyles.None, out var result))
		{
			throw new SerializationException();
		}
		input.Remove(0, array.Length);
		return result;
	}

	private static DateTime DeserializeTime(ref StringBuilder input)
	{
		if (input[8] == ';')
		{
			char[] array = new char[9];
			input.CopyTo(0, array, 0, array.Length);
			if (!DateTime.TryParseExact(new string(array), "HH:mm:ss;", CultureInfo.InvariantCulture, DateTimeStyles.NoCurrentDateDefault, out var result))
			{
				throw new SerializationException();
			}
			input.Remove(0, array.Length);
			return result;
		}
		if (input[12] == ';')
		{
			char[] array2 = new char[13];
			input.CopyTo(0, array2, 0, array2.Length);
			if (!DateTime.TryParseExact(new string(array2), "HH:mm:ss.fff;", CultureInfo.InvariantCulture, DateTimeStyles.NoCurrentDateDefault, out var result2))
			{
				throw new SerializationException();
			}
			input.Remove(0, array2.Length);
			return result2;
		}
		throw new SerializationException();
	}

	private static string EscapeForSerialization(string unescaped)
	{
		return unescaped.Replace("\\", "\\\\").Replace(";", "\\;");
	}

	private static string SerializeTransitionTime(TransitionTime transition)
	{
		string text = ((transition.TimeOfDay.Millisecond <= 0) ? transition.TimeOfDay.ToString("HH:mm:ss") : transition.TimeOfDay.ToString("HH:mm:ss.fff"));
		if (transition.IsFixedDateRule)
		{
			return $"[1;{text};{transition.Month};{transition.Day};]";
		}
		return $"[0;{text};{transition.Month};{transition.Week};{(int)transition.DayOfWeek};]";
	}

	private static List<AdjustmentRule> CreateAdjustmentRule(int year, out long[] data, out string[] names, string standardNameCurrentYear, string daylightNameCurrentYear)
	{
		List<AdjustmentRule> list = new List<AdjustmentRule>();
		if (!CurrentSystemTimeZone.GetTimeZoneData(year, out data, out names, out var daylight_inverted))
		{
			return list;
		}
		DateTime dateTime = new DateTime(data[0]);
		DateTime value = new DateTime(data[1]);
		TimeSpan daylightDelta = new TimeSpan(data[3]);
		if (standardNameCurrentYear != names[0])
		{
			return list;
		}
		if (daylightNameCurrentYear != names[1])
		{
			return list;
		}
		if (dateTime.Equals(value))
		{
			return list;
		}
		DateTime dateStart = new DateTime(year, 1, 1, 0, 0, 0, 0);
		DateTime dateEnd = new DateTime(year, 12, DateTime.DaysInMonth(year, 12));
		DateTime dateTime2 = new DateTime(year, 12, DateTime.DaysInMonth(year, 12), 23, 59, 59, 999);
		if (!daylight_inverted)
		{
			TransitionTime daylightTransitionStart = TransitionTime.CreateFixedDateRule(new DateTime(1, 1, 1).Add(dateTime.TimeOfDay), dateTime.Month, dateTime.Day);
			TransitionTime daylightTransitionEnd = TransitionTime.CreateFixedDateRule(new DateTime(1, 1, 1).Add(value.TimeOfDay), value.Month, value.Day);
			AdjustmentRule item = AdjustmentRule.CreateAdjustmentRule(dateStart, dateEnd, daylightDelta, daylightTransitionStart, daylightTransitionEnd);
			list.Add(item);
		}
		else
		{
			TransitionTime daylightTransitionStart2 = TransitionTime.CreateFixedDateRule(new DateTime(1, 1, 1), 1, 1);
			TransitionTime daylightTransitionEnd2 = TransitionTime.CreateFixedDateRule(new DateTime(1, 1, 1).Add(dateTime.TimeOfDay), dateTime.Month, dateTime.Day);
			AdjustmentRule item2 = AdjustmentRule.CreateAdjustmentRule(new DateTime(year, 1, 1), new DateTime(dateTime.Year, dateTime.Month, dateTime.Day), daylightDelta, daylightTransitionStart2, daylightTransitionEnd2);
			list.Add(item2);
			TransitionTime daylightTransitionStart3 = TransitionTime.CreateFixedDateRule(new DateTime(1, 1, 1).Add(value.TimeOfDay), value.Month, value.Day);
			TransitionTime daylightTransitionEnd3 = TransitionTime.CreateFixedDateRule(new DateTime(1, 1, 1).Add(dateTime2.TimeOfDay), dateTime2.Month, dateTime2.Day);
			AdjustmentRule item3 = AdjustmentRule.CreateAdjustmentRule(new DateTime(dateTime.Year, dateTime.Month, dateTime.Day).AddDays(1.0), dateEnd, daylightDelta, daylightTransitionStart3, daylightTransitionEnd3);
			list.Add(item3);
		}
		return list;
	}

	private static TimeZoneInfo CreateLocalUnity()
	{
		int year = DateTime.UtcNow.Year;
		if (!CurrentSystemTimeZone.GetTimeZoneData(year, out var data, out var names, out var _))
		{
			throw new NotSupportedException("Can't get timezone name.");
		}
		TimeSpan timeSpan = TimeSpan.FromTicks(data[2]);
		string text = "(GMT" + ((timeSpan >= TimeSpan.Zero) ? '+' : '-') + timeSpan.ToString("hh\\:mm") + ") Local Time";
		string standardNameCurrentYear = names[0];
		string daylightNameCurrentYear = names[1];
		List<AdjustmentRule> list = new List<AdjustmentRule>();
		bool flag = data[3] == 0;
		if (!flag)
		{
			int num = 1971;
			int num2 = 2037;
			for (int i = year; i <= num2; i++)
			{
				List<AdjustmentRule> list2 = CreateAdjustmentRule(i, out data, out names, standardNameCurrentYear, daylightNameCurrentYear);
				if (list2.Count <= 0)
				{
					break;
				}
				list.AddRange(list2);
			}
			for (int num3 = year - 1; num3 >= num; num3--)
			{
				List<AdjustmentRule> list3 = CreateAdjustmentRule(num3, out data, out names, standardNameCurrentYear, daylightNameCurrentYear);
				if (list3.Count <= 0)
				{
					break;
				}
				list.AddRange(list3);
			}
			list.Sort((AdjustmentRule rule1, AdjustmentRule rule2) => rule1.DateStart.CompareTo(rule2.DateStart));
		}
		return CreateCustomTimeZone("Local", timeSpan, text, standardNameCurrentYear, daylightNameCurrentYear, list.ToArray(), flag);
	}

	[DllImport("api-ms-win-core-timezone-l1-1-0.dll")]
	internal static extern uint EnumDynamicTimeZoneInformation(uint dwIndex, out DYNAMIC_TIME_ZONE_INFORMATION lpTimeZoneInformation);

	[DllImport("api-ms-win-core-timezone-l1-1-0.dll")]
	internal static extern uint GetDynamicTimeZoneInformation(out DYNAMIC_TIME_ZONE_INFORMATION pTimeZoneInformation);

	[DllImport("kernel32.dll", EntryPoint = "GetDynamicTimeZoneInformation")]
	internal static extern uint GetDynamicTimeZoneInformationWin32(out DYNAMIC_TIME_ZONE_INFORMATION pTimeZoneInformation);

	[DllImport("api-ms-win-core-timezone-l1-1-0.dll")]
	internal static extern uint GetDynamicTimeZoneInformationEffectiveYears(ref DYNAMIC_TIME_ZONE_INFORMATION lpTimeZoneInformation, out uint FirstYear, out uint LastYear);

	[DllImport("api-ms-win-core-timezone-l1-1-0.dll")]
	internal static extern bool GetTimeZoneInformationForYear(ushort wYear, ref DYNAMIC_TIME_ZONE_INFORMATION pdtzi, out TIME_ZONE_INFORMATION ptzi);

	internal static AdjustmentRule CreateAdjustmentRuleFromTimeZoneInformation(ref DYNAMIC_TIME_ZONE_INFORMATION timeZoneInformation, DateTime startDate, DateTime endDate, int defaultBaseUtcOffset)
	{
		if (timeZoneInformation.TZI.StandardDate.wMonth == 0)
		{
			if (timeZoneInformation.TZI.Bias == defaultBaseUtcOffset)
			{
				return null;
			}
			return AdjustmentRule.CreateAdjustmentRule(startDate, endDate, TimeSpan.Zero, TransitionTime.CreateFixedDateRule(DateTime.MinValue, 1, 1), TransitionTime.CreateFixedDateRule(DateTime.MinValue.AddMilliseconds(1.0), 1, 1), new TimeSpan(0, defaultBaseUtcOffset - timeZoneInformation.TZI.Bias, 0));
		}
		if (!TransitionTimeFromTimeZoneInformation(timeZoneInformation, out var transitionTime, readStartDate: true))
		{
			return null;
		}
		if (!TransitionTimeFromTimeZoneInformation(timeZoneInformation, out var transitionTime2, readStartDate: false))
		{
			return null;
		}
		if (transitionTime.Equals(transitionTime2))
		{
			return null;
		}
		return AdjustmentRule.CreateAdjustmentRule(startDate, endDate, new TimeSpan(0, -timeZoneInformation.TZI.DaylightBias, 0), transitionTime, transitionTime2, new TimeSpan(0, defaultBaseUtcOffset - timeZoneInformation.TZI.Bias, 0));
	}

	private static bool TransitionTimeFromTimeZoneInformation(DYNAMIC_TIME_ZONE_INFORMATION timeZoneInformation, out TransitionTime transitionTime, bool readStartDate)
	{
		if (timeZoneInformation.TZI.StandardDate.wMonth == 0)
		{
			transitionTime = default(TransitionTime);
			return false;
		}
		if (readStartDate)
		{
			if (timeZoneInformation.TZI.DaylightDate.wYear == 0)
			{
				transitionTime = TransitionTime.CreateFloatingDateRule(new DateTime(1, 1, 1, timeZoneInformation.TZI.DaylightDate.wHour, timeZoneInformation.TZI.DaylightDate.wMinute, timeZoneInformation.TZI.DaylightDate.wSecond, timeZoneInformation.TZI.DaylightDate.wMilliseconds), timeZoneInformation.TZI.DaylightDate.wMonth, timeZoneInformation.TZI.DaylightDate.wDay, (DayOfWeek)timeZoneInformation.TZI.DaylightDate.wDayOfWeek);
			}
			else
			{
				transitionTime = TransitionTime.CreateFixedDateRule(new DateTime(1, 1, 1, timeZoneInformation.TZI.DaylightDate.wHour, timeZoneInformation.TZI.DaylightDate.wMinute, timeZoneInformation.TZI.DaylightDate.wSecond, timeZoneInformation.TZI.DaylightDate.wMilliseconds), timeZoneInformation.TZI.DaylightDate.wMonth, timeZoneInformation.TZI.DaylightDate.wDay);
			}
		}
		else if (timeZoneInformation.TZI.StandardDate.wYear == 0)
		{
			transitionTime = TransitionTime.CreateFloatingDateRule(new DateTime(1, 1, 1, timeZoneInformation.TZI.StandardDate.wHour, timeZoneInformation.TZI.StandardDate.wMinute, timeZoneInformation.TZI.StandardDate.wSecond, timeZoneInformation.TZI.StandardDate.wMilliseconds), timeZoneInformation.TZI.StandardDate.wMonth, timeZoneInformation.TZI.StandardDate.wDay, (DayOfWeek)timeZoneInformation.TZI.StandardDate.wDayOfWeek);
		}
		else
		{
			transitionTime = TransitionTime.CreateFixedDateRule(new DateTime(1, 1, 1, timeZoneInformation.TZI.StandardDate.wHour, timeZoneInformation.TZI.StandardDate.wMinute, timeZoneInformation.TZI.StandardDate.wSecond, timeZoneInformation.TZI.StandardDate.wMilliseconds), timeZoneInformation.TZI.StandardDate.wMonth, timeZoneInformation.TZI.StandardDate.wDay);
		}
		return true;
	}

	internal static TimeZoneInfo TryCreateTimeZone(DYNAMIC_TIME_ZONE_INFORMATION timeZoneInformation)
	{
		uint FirstYear = 0u;
		uint LastYear = 0u;
		AdjustmentRule[] array = null;
		int bias = timeZoneInformation.TZI.Bias;
		if (string.IsNullOrEmpty(timeZoneInformation.TimeZoneKeyName))
		{
			return null;
		}
		try
		{
			if (GetDynamicTimeZoneInformationEffectiveYears(ref timeZoneInformation, out FirstYear, out LastYear) != 0)
			{
				FirstYear = (LastYear = 0u);
			}
		}
		catch
		{
			FirstYear = (LastYear = 0u);
		}
		if (FirstYear == LastYear)
		{
			AdjustmentRule adjustmentRule = CreateAdjustmentRuleFromTimeZoneInformation(ref timeZoneInformation, DateTime.MinValue.Date, DateTime.MaxValue.Date, bias);
			if (adjustmentRule != null)
			{
				array = new AdjustmentRule[1] { adjustmentRule };
			}
		}
		else
		{
			DYNAMIC_TIME_ZONE_INFORMATION timeZoneInformation2 = default(DYNAMIC_TIME_ZONE_INFORMATION);
			List<AdjustmentRule> list = new List<AdjustmentRule>();
			if (!GetTimeZoneInformationForYear((ushort)FirstYear, ref timeZoneInformation, out timeZoneInformation2.TZI))
			{
				return null;
			}
			AdjustmentRule adjustmentRule = CreateAdjustmentRuleFromTimeZoneInformation(ref timeZoneInformation2, DateTime.MinValue.Date, new DateTime((int)FirstYear, 12, 31), bias);
			if (adjustmentRule != null)
			{
				list.Add(adjustmentRule);
			}
			for (uint num = FirstYear + 1; num < LastYear; num++)
			{
				if (!GetTimeZoneInformationForYear((ushort)num, ref timeZoneInformation, out timeZoneInformation2.TZI))
				{
					return null;
				}
				adjustmentRule = CreateAdjustmentRuleFromTimeZoneInformation(ref timeZoneInformation2, new DateTime((int)num, 1, 1), new DateTime((int)num, 12, 31), bias);
				if (adjustmentRule != null)
				{
					list.Add(adjustmentRule);
				}
			}
			if (!GetTimeZoneInformationForYear((ushort)LastYear, ref timeZoneInformation, out timeZoneInformation2.TZI))
			{
				return null;
			}
			adjustmentRule = CreateAdjustmentRuleFromTimeZoneInformation(ref timeZoneInformation2, new DateTime((int)LastYear, 1, 1), DateTime.MaxValue.Date, bias);
			if (adjustmentRule != null)
			{
				list.Add(adjustmentRule);
			}
			if (list.Count > 0)
			{
				array = list.ToArray();
			}
		}
		return new TimeZoneInfo(timeZoneInformation.TimeZoneKeyName, new TimeSpan(0, -timeZoneInformation.TZI.Bias, 0), timeZoneInformation.TZI.StandardName, timeZoneInformation.TZI.StandardName, timeZoneInformation.TZI.DaylightName, array, disableDaylightSavingTime: false);
	}

	internal static TimeZoneInfo GetLocalTimeZoneInfoWinRTFallback()
	{
		try
		{
			if (GetDynamicTimeZoneInformation(out var pTimeZoneInformation) == uint.MaxValue)
			{
				return Utc;
			}
			TimeZoneInfo timeZoneInfo = TryCreateTimeZone(pTimeZoneInformation);
			return (timeZoneInfo != null) ? timeZoneInfo : Utc;
		}
		catch
		{
			return Utc;
		}
	}

	internal static string GetLocalTimeZoneKeyNameWin32Fallback()
	{
		try
		{
			if (GetDynamicTimeZoneInformationWin32(out var pTimeZoneInformation) == uint.MaxValue)
			{
				return null;
			}
			if (!string.IsNullOrEmpty(pTimeZoneInformation.TimeZoneKeyName))
			{
				return pTimeZoneInformation.TimeZoneKeyName;
			}
			if (!string.IsNullOrEmpty(pTimeZoneInformation.TZI.StandardName))
			{
				return pTimeZoneInformation.TZI.StandardName;
			}
			return null;
		}
		catch
		{
			return null;
		}
	}

	internal static TimeZoneInfo FindSystemTimeZoneByIdWinRTFallback(string id)
	{
		foreach (TimeZoneInfo systemTimeZone in GetSystemTimeZones())
		{
			if (string.Compare(id, systemTimeZone.Id, StringComparison.Ordinal) == 0)
			{
				return systemTimeZone;
			}
		}
		throw new TimeZoneNotFoundException();
	}

	internal static List<TimeZoneInfo> GetSystemTimeZonesWinRTFallback()
	{
		List<TimeZoneInfo> list = new List<TimeZoneInfo>();
		try
		{
			uint num = 0u;
			DYNAMIC_TIME_ZONE_INFORMATION lpTimeZoneInformation;
			while (EnumDynamicTimeZoneInformation(num++, out lpTimeZoneInformation) == 0)
			{
				TimeZoneInfo timeZoneInfo = TryCreateTimeZone(lpTimeZoneInformation);
				if (timeZoneInfo != null)
				{
					list.Add(timeZoneInfo);
				}
			}
		}
		catch
		{
		}
		if (list.Count == 0)
		{
			TimeZoneInfo localTimeZoneInfoWinRTFallback = GetLocalTimeZoneInfoWinRTFallback();
			if (Interlocked.CompareExchange(ref local, localTimeZoneInfoWinRTFallback, null) != null)
			{
				localTimeZoneInfoWinRTFallback = local;
			}
			list.Add(localTimeZoneInfoWinRTFallback);
		}
		return list;
	}

	[DllImport("libc")]
	private static extern int readlink(string path, byte[] buffer, int buflen);

	private static string readlink(string path)
	{
		if (readlinkNotFound)
		{
			return null;
		}
		byte[] array = new byte[512];
		int num;
		try
		{
			num = readlink(path, array, array.Length);
		}
		catch (DllNotFoundException)
		{
			readlinkNotFound = true;
			return null;
		}
		catch (EntryPointNotFoundException)
		{
			readlinkNotFound = true;
			return null;
		}
		if (num == -1)
		{
			return null;
		}
		char[] array2 = new char[512];
		int chars = Encoding.Default.GetChars(array, 0, num, array2, 0);
		return new string(array2, 0, chars);
	}

	private static bool TryGetNameFromPath(string path, out string name)
	{
		name = null;
		if (!File.Exists(path))
		{
			return false;
		}
		string text = readlink(path);
		if (text != null)
		{
			path = ((!Path.IsPathRooted(text)) ? Path.Combine(Path.GetDirectoryName(path), text) : text);
		}
		path = Path.GetFullPath(path);
		if (string.IsNullOrEmpty(TimeZoneDirectory))
		{
			return false;
		}
		string text2 = TimeZoneDirectory;
		if (text2[text2.Length - 1] != Path.DirectorySeparatorChar)
		{
			text2 += Path.DirectorySeparatorChar;
		}
		if (!path.StartsWith(text2, StringComparison.InvariantCulture))
		{
			return false;
		}
		name = path.Substring(text2.Length);
		if (name == "localtime")
		{
			name = "Local";
		}
		return true;
	}

	private static TimeZoneInfo CreateLocal()
	{
		if (IsWindows && LocalZoneKey != null)
		{
			string text = (string)LocalZoneKey.GetValue("TimeZoneKeyName");
			if (text == null)
			{
				text = (string)LocalZoneKey.GetValue("StandardName");
			}
			text = TrimSpecial(text);
			if (string.IsNullOrEmpty(text))
			{
				text = GetLocalTimeZoneKeyNameWin32Fallback();
			}
			if (text != null)
			{
				try
				{
					return FindSystemTimeZoneById(text);
				}
				catch (TimeZoneNotFoundException)
				{
					return GetLocalTimeZoneInfoWinRTFallback();
				}
			}
		}
		else if (IsWindows)
		{
			return GetLocalTimeZoneInfoWinRTFallback();
		}
		TimeZoneInfo timeZoneInfo = null;
		try
		{
			timeZoneInfo = CreateLocalUnity();
		}
		catch
		{
			timeZoneInfo = null;
		}
		if (timeZoneInfo == null)
		{
			timeZoneInfo = Utc;
		}
		string environmentVariable = Environment.GetEnvironmentVariable("TZ");
		if (environmentVariable != null)
		{
			if (environmentVariable == string.Empty)
			{
				return timeZoneInfo;
			}
			try
			{
				return FindSystemTimeZoneByFileName(environmentVariable, Path.Combine(TimeZoneDirectory, environmentVariable));
			}
			catch
			{
				return timeZoneInfo;
			}
		}
		string[] array = new string[2]
		{
			"/etc/localtime",
			Path.Combine(TimeZoneDirectory, "localtime")
		};
		foreach (string text2 in array)
		{
			try
			{
				string name = null;
				if (!TryGetNameFromPath(text2, out name))
				{
					name = "Local";
				}
				if (File.Exists(text2))
				{
					return FindSystemTimeZoneByFileName(name, text2);
				}
			}
			catch (TimeZoneNotFoundException)
			{
			}
		}
		return timeZoneInfo;
	}

	private static TimeZoneInfo FindSystemTimeZoneByIdCore(string id)
	{
		string filepath = Path.Combine(TimeZoneDirectory, id);
		return FindSystemTimeZoneByFileName(id, filepath);
	}

	private static void GetSystemTimeZonesCore(List<TimeZoneInfo> systemTimeZones)
	{
		string[] subKeyNames;
		if (TimeZoneKey != null)
		{
			subKeyNames = TimeZoneKey.GetSubKeyNames();
			foreach (string name in subKeyNames)
			{
				using (RegistryKey registryKey = TimeZoneKey.OpenSubKey(name))
				{
					if (registryKey == null || registryKey.GetValue("TZI") == null)
					{
						continue;
					}
					goto IL_0044;
				}
				IL_0044:
				systemTimeZones.Add(FindSystemTimeZoneById(name));
			}
			return;
		}
		if (IsWindows)
		{
			systemTimeZones.AddRange(GetSystemTimeZonesWinRTFallback());
			return;
		}
		subKeyNames = new string[16]
		{
			"Africa", "America", "Antarctica", "Arctic", "Asia", "Atlantic", "Australia", "Brazil", "Canada", "Chile",
			"Europe", "Indian", "Mexico", "Mideast", "Pacific", "US"
		};
		foreach (string text in subKeyNames)
		{
			try
			{
				string[] files = Directory.GetFiles(Path.Combine(TimeZoneDirectory, text));
				foreach (string path in files)
				{
					try
					{
						string text2 = $"{text}/{Path.GetFileName(path)}";
						systemTimeZones.Add(FindSystemTimeZoneById(text2));
					}
					catch (ArgumentNullException)
					{
					}
					catch (TimeZoneNotFoundException)
					{
					}
					catch (InvalidTimeZoneException)
					{
					}
					catch (Exception)
					{
						throw;
					}
				}
			}
			catch
			{
			}
		}
	}

	private static string TrimSpecial(string str)
	{
		if (str == null)
		{
			return str;
		}
		int i;
		for (i = 0; i < str.Length && !char.IsLetterOrDigit(str[i]); i++)
		{
		}
		int num = str.Length - 1;
		while (num > i && !char.IsLetterOrDigit(str[num]) && str[num] != ')')
		{
			num--;
		}
		return str.Substring(i, num - i + 1);
	}

	private static bool TryAddTicks(DateTime date, long ticks, out DateTime result, DateTimeKind kind = DateTimeKind.Unspecified)
	{
		long num = date.Ticks + ticks;
		if (num < DateTime.MinValue.Ticks)
		{
			result = DateTime.SpecifyKind(DateTime.MinValue, kind);
			return false;
		}
		if (num > DateTime.MaxValue.Ticks)
		{
			result = DateTime.SpecifyKind(DateTime.MaxValue, kind);
			return false;
		}
		result = new DateTime(num, kind);
		return true;
	}

	public static void ClearCachedData()
	{
		local = null;
		utc = null;
		systemTimeZones = null;
	}

	public static DateTime ConvertTime(DateTime dateTime, TimeZoneInfo destinationTimeZone)
	{
		return ConvertTime(dateTime, (dateTime.Kind == DateTimeKind.Utc) ? Utc : Local, destinationTimeZone);
	}

	public static DateTime ConvertTime(DateTime dateTime, TimeZoneInfo sourceTimeZone, TimeZoneInfo destinationTimeZone)
	{
		if (sourceTimeZone == null)
		{
			throw new ArgumentNullException("sourceTimeZone");
		}
		if (destinationTimeZone == null)
		{
			throw new ArgumentNullException("destinationTimeZone");
		}
		if (dateTime.Kind == DateTimeKind.Local && sourceTimeZone != Local)
		{
			throw new ArgumentException("Kind property of dateTime is Local but the sourceTimeZone does not equal TimeZoneInfo.Local");
		}
		if (dateTime.Kind == DateTimeKind.Utc && sourceTimeZone != Utc)
		{
			throw new ArgumentException("Kind property of dateTime is Utc but the sourceTimeZone does not equal TimeZoneInfo.Utc");
		}
		if (sourceTimeZone.IsInvalidTime(dateTime))
		{
			throw new ArgumentException("dateTime parameter is an invalid time");
		}
		if (dateTime.Kind == DateTimeKind.Local && sourceTimeZone == Local && destinationTimeZone == Local)
		{
			return dateTime;
		}
		DateTime dateTime2 = ConvertTimeToUtc(dateTime, sourceTimeZone);
		if (destinationTimeZone != Utc)
		{
			dateTime2 = ConvertTimeFromUtc(dateTime2, destinationTimeZone);
			if (dateTime.Kind == DateTimeKind.Unspecified)
			{
				return DateTime.SpecifyKind(dateTime2, DateTimeKind.Unspecified);
			}
		}
		return dateTime2;
	}

	public static DateTimeOffset ConvertTime(DateTimeOffset dateTimeOffset, TimeZoneInfo destinationTimeZone)
	{
		if (destinationTimeZone == null)
		{
			throw new ArgumentNullException("destinationTimeZone");
		}
		DateTime utcDateTime = dateTimeOffset.UtcDateTime;
		bool isDST;
		TimeSpan utcOffset = destinationTimeZone.GetUtcOffset(utcDateTime, out isDST);
		return new DateTimeOffset(DateTime.SpecifyKind(utcDateTime, DateTimeKind.Unspecified) + utcOffset, utcOffset);
	}

	public static DateTime ConvertTimeBySystemTimeZoneId(DateTime dateTime, string destinationTimeZoneId)
	{
		return ConvertTime(dateTime, FindSystemTimeZoneById(destinationTimeZoneId));
	}

	public static DateTime ConvertTimeBySystemTimeZoneId(DateTime dateTime, string sourceTimeZoneId, string destinationTimeZoneId)
	{
		return ConvertTime(sourceTimeZone: (dateTime.Kind != DateTimeKind.Utc || !(sourceTimeZoneId == Utc.Id)) ? FindSystemTimeZoneById(sourceTimeZoneId) : Utc, dateTime: dateTime, destinationTimeZone: FindSystemTimeZoneById(destinationTimeZoneId));
	}

	public static DateTimeOffset ConvertTimeBySystemTimeZoneId(DateTimeOffset dateTimeOffset, string destinationTimeZoneId)
	{
		return ConvertTime(dateTimeOffset, FindSystemTimeZoneById(destinationTimeZoneId));
	}

	private DateTime ConvertTimeFromUtc(DateTime dateTime)
	{
		if (dateTime.Kind == DateTimeKind.Local)
		{
			throw new ArgumentException("Kind property of dateTime is Local");
		}
		if (this == Utc)
		{
			return DateTime.SpecifyKind(dateTime, DateTimeKind.Utc);
		}
		TimeSpan utcOffset = GetUtcOffset(dateTime);
		DateTimeKind kind = ((this == Local) ? DateTimeKind.Local : DateTimeKind.Unspecified);
		if (!TryAddTicks(dateTime, utcOffset.Ticks, out var result, kind))
		{
			return DateTime.SpecifyKind(DateTime.MaxValue, kind);
		}
		return result;
	}

	public static DateTime ConvertTimeFromUtc(DateTime dateTime, TimeZoneInfo destinationTimeZone)
	{
		if (destinationTimeZone == null)
		{
			throw new ArgumentNullException("destinationTimeZone");
		}
		return destinationTimeZone.ConvertTimeFromUtc(dateTime);
	}

	public static DateTime ConvertTimeToUtc(DateTime dateTime)
	{
		if (dateTime.Kind == DateTimeKind.Utc)
		{
			return dateTime;
		}
		return ConvertTimeToUtc(dateTime, Local);
	}

	internal static DateTime ConvertTimeToUtc(DateTime dateTime, TimeZoneInfoOptions flags)
	{
		return ConvertTimeToUtc(dateTime, Local, flags);
	}

	public static DateTime ConvertTimeToUtc(DateTime dateTime, TimeZoneInfo sourceTimeZone)
	{
		return ConvertTimeToUtc(dateTime, sourceTimeZone, TimeZoneInfoOptions.None);
	}

	private static DateTime ConvertTimeToUtc(DateTime dateTime, TimeZoneInfo sourceTimeZone, TimeZoneInfoOptions flags)
	{
		if ((flags & TimeZoneInfoOptions.NoThrowOnInvalidTime) == 0)
		{
			if (sourceTimeZone == null)
			{
				throw new ArgumentNullException("sourceTimeZone");
			}
			if (dateTime.Kind == DateTimeKind.Utc && sourceTimeZone != Utc)
			{
				throw new ArgumentException("Kind property of dateTime is Utc but the sourceTimeZone does not equal TimeZoneInfo.Utc");
			}
			if (dateTime.Kind == DateTimeKind.Local && sourceTimeZone != Local)
			{
				throw new ArgumentException("Kind property of dateTime is Local but the sourceTimeZone does not equal TimeZoneInfo.Local");
			}
			if (sourceTimeZone.IsInvalidTime(dateTime))
			{
				throw new ArgumentException("dateTime parameter is an invalid time");
			}
		}
		if (dateTime.Kind == DateTimeKind.Utc)
		{
			return dateTime;
		}
		bool isDST;
		TimeSpan utcOffset = sourceTimeZone.GetUtcOffset(dateTime, out isDST);
		TryAddTicks(dateTime, -utcOffset.Ticks, out var result, DateTimeKind.Utc);
		return result;
	}

	internal static TimeSpan GetDateTimeNowUtcOffsetFromUtc(DateTime time, out bool isAmbiguousLocalDst)
	{
		bool isDaylightSavings;
		return GetUtcOffsetFromUtc(time, Local, out isDaylightSavings, out isAmbiguousLocalDst);
	}

	public static TimeZoneInfo CreateCustomTimeZone(string id, TimeSpan baseUtcOffset, string displayName, string standardDisplayName)
	{
		return CreateCustomTimeZone(id, baseUtcOffset, displayName, standardDisplayName, null, null, disableDaylightSavingTime: true);
	}

	public static TimeZoneInfo CreateCustomTimeZone(string id, TimeSpan baseUtcOffset, string displayName, string standardDisplayName, string daylightDisplayName, AdjustmentRule[] adjustmentRules)
	{
		return CreateCustomTimeZone(id, baseUtcOffset, displayName, standardDisplayName, daylightDisplayName, adjustmentRules, disableDaylightSavingTime: false);
	}

	public static TimeZoneInfo CreateCustomTimeZone(string id, TimeSpan baseUtcOffset, string displayName, string standardDisplayName, string daylightDisplayName, AdjustmentRule[] adjustmentRules, bool disableDaylightSavingTime)
	{
		return new TimeZoneInfo(id, baseUtcOffset, displayName, standardDisplayName, daylightDisplayName, adjustmentRules, disableDaylightSavingTime);
	}

	public override bool Equals(object obj)
	{
		return Equals(obj as TimeZoneInfo);
	}

	public bool Equals(TimeZoneInfo other)
	{
		if (other == null)
		{
			return false;
		}
		if (other.Id == Id)
		{
			return HasSameRules(other);
		}
		return false;
	}

	public static TimeZoneInfo FindSystemTimeZoneById(string id)
	{
		if (id == null)
		{
			throw new ArgumentNullException("id");
		}
		if (TimeZoneKey != null)
		{
			if (id == "Coordinated Universal Time")
			{
				id = "UTC";
			}
			RegistryKey registryKey = TimeZoneKey.OpenSubKey(id, writable: false);
			if (registryKey == null)
			{
				throw new TimeZoneNotFoundException();
			}
			return FromRegistryKey(id, registryKey);
		}
		if (IsWindows)
		{
			return FindSystemTimeZoneByIdWinRTFallback(id);
		}
		if (id == "Local")
		{
			return Local;
		}
		return FindSystemTimeZoneByIdCore(id);
	}

	private static TimeZoneInfo FindSystemTimeZoneByFileName(string id, string filepath)
	{
		FileStream fileStream = null;
		try
		{
			fileStream = File.OpenRead(filepath);
		}
		catch (Exception innerException)
		{
			throw new TimeZoneNotFoundException("Couldn't read time zone file " + filepath, innerException);
		}
		try
		{
			return BuildFromStream(id, fileStream);
		}
		finally
		{
			fileStream?.Dispose();
		}
	}

	private static TimeZoneInfo FromRegistryKey(string id, RegistryKey key)
	{
		byte[] array = (byte[])key.GetValue("TZI");
		if (array == null)
		{
			throw new InvalidTimeZoneException();
		}
		int num = BitConverter.ToInt32(array, 0);
		TimeSpan timeSpan = new TimeSpan(0, -num, 0);
		string text = (string)key.GetValue("Display");
		string text2 = (string)key.GetValue("Std");
		string text3 = (string)key.GetValue("Dlt");
		List<AdjustmentRule> list = new List<AdjustmentRule>();
		RegistryKey registryKey = key.OpenSubKey("Dynamic DST", writable: false);
		if (registryKey != null)
		{
			int num2 = (int)registryKey.GetValue("FirstEntry");
			int num3 = (int)registryKey.GetValue("LastEntry");
			for (int i = num2; i <= num3; i++)
			{
				byte[] array2 = (byte[])registryKey.GetValue(i.ToString());
				if (array2 != null)
				{
					int start_year = ((i == num2) ? 1 : i);
					int end_year = ((i == num3) ? 9999 : i);
					ParseRegTzi(list, start_year, end_year, array2);
				}
			}
		}
		else
		{
			ParseRegTzi(list, 1, 9999, array);
		}
		return CreateCustomTimeZone(id, timeSpan, text, text2, text3, ValidateRules(list));
	}

	private static void ParseRegTzi(List<AdjustmentRule> adjustmentRules, int start_year, int end_year, byte[] buffer)
	{
		int num = BitConverter.ToInt32(buffer, 8);
		int num2 = BitConverter.ToInt16(buffer, 12);
		int num3 = BitConverter.ToInt16(buffer, 14);
		int dayOfWeek = BitConverter.ToInt16(buffer, 16);
		int num4 = BitConverter.ToInt16(buffer, 18);
		int hour = BitConverter.ToInt16(buffer, 20);
		int minute = BitConverter.ToInt16(buffer, 22);
		int second = BitConverter.ToInt16(buffer, 24);
		int millisecond = BitConverter.ToInt16(buffer, 26);
		int num5 = BitConverter.ToInt16(buffer, 28);
		int num6 = BitConverter.ToInt16(buffer, 30);
		int dayOfWeek2 = BitConverter.ToInt16(buffer, 32);
		int num7 = BitConverter.ToInt16(buffer, 34);
		int hour2 = BitConverter.ToInt16(buffer, 36);
		int minute2 = BitConverter.ToInt16(buffer, 38);
		int second2 = BitConverter.ToInt16(buffer, 40);
		int millisecond2 = BitConverter.ToInt16(buffer, 42);
		if (num3 != 0 && num6 != 0)
		{
			DateTime timeOfDay = new DateTime(1, 1, 1, hour2, minute2, second2, millisecond2);
			DateTime dateStart = new DateTime(start_year, 1, 1);
			TransitionTime daylightTransitionStart = ((num5 != 0) ? TransitionTime.CreateFixedDateRule(timeOfDay, num6, num7) : TransitionTime.CreateFloatingDateRule(timeOfDay, num6, num7, (DayOfWeek)dayOfWeek2));
			DateTime timeOfDay2 = new DateTime(1, 1, 1, hour, minute, second, millisecond);
			DateTime dateEnd = new DateTime(end_year, 12, 31);
			TransitionTime daylightTransitionEnd = ((num2 != 0) ? TransitionTime.CreateFixedDateRule(timeOfDay2, num3, num4) : TransitionTime.CreateFloatingDateRule(timeOfDay2, num3, num4, (DayOfWeek)dayOfWeek));
			TimeSpan daylightDelta = new TimeSpan(0, -num, 0);
			adjustmentRules.Add(AdjustmentRule.CreateAdjustmentRule(dateStart, dateEnd, daylightDelta, daylightTransitionStart, daylightTransitionEnd));
		}
	}

	public AdjustmentRule[] GetAdjustmentRules()
	{
		if (!supportsDaylightSavingTime || adjustmentRules == null)
		{
			return new AdjustmentRule[0];
		}
		return (AdjustmentRule[])adjustmentRules.Clone();
	}

	public TimeSpan[] GetAmbiguousTimeOffsets(DateTime dateTime)
	{
		if (!IsAmbiguousTime(dateTime))
		{
			throw new ArgumentException("dateTime is not an ambiguous time");
		}
		AdjustmentRule applicableRule = GetApplicableRule(dateTime);
		if (applicableRule == null)
		{
			return new TimeSpan[2] { baseUtcOffset, baseUtcOffset };
		}
		return new TimeSpan[2]
		{
			baseUtcOffset,
			baseUtcOffset + applicableRule.DaylightDelta
		};
	}

	public TimeSpan[] GetAmbiguousTimeOffsets(DateTimeOffset dateTimeOffset)
	{
		if (!IsAmbiguousTime(dateTimeOffset))
		{
			throw new ArgumentException("dateTimeOffset is not an ambiguous time");
		}
		throw new NotImplementedException();
	}

	public override int GetHashCode()
	{
		int num = Id.GetHashCode();
		AdjustmentRule[] array = GetAdjustmentRules();
		foreach (AdjustmentRule adjustmentRule in array)
		{
			num ^= adjustmentRule.GetHashCode();
		}
		return num;
	}

	void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		info.AddValue("Id", id);
		info.AddValue("DisplayName", displayName);
		info.AddValue("StandardName", standardDisplayName);
		info.AddValue("DaylightName", daylightDisplayName);
		info.AddValue("BaseUtcOffset", baseUtcOffset);
		info.AddValue("AdjustmentRules", adjustmentRules);
		info.AddValue("SupportsDaylightSavingTime", SupportsDaylightSavingTime);
	}

	public static ReadOnlyCollection<TimeZoneInfo> GetSystemTimeZones()
	{
		if (systemTimeZones == null)
		{
			List<TimeZoneInfo> list = new List<TimeZoneInfo>();
			GetSystemTimeZonesCore(list);
			Interlocked.CompareExchange(ref systemTimeZones, new ReadOnlyCollection<TimeZoneInfo>(list), null);
		}
		return systemTimeZones;
	}

	public TimeSpan GetUtcOffset(DateTime dateTime)
	{
		bool isDST;
		return GetUtcOffset(dateTime, out isDST);
	}

	public TimeSpan GetUtcOffset(DateTimeOffset dateTimeOffset)
	{
		bool isDST;
		return GetUtcOffset(dateTimeOffset.UtcDateTime, out isDST);
	}

	private TimeSpan GetUtcOffset(DateTime dateTime, out bool isDST)
	{
		isDST = false;
		TimeZoneInfo timeZoneInfo = this;
		if (dateTime.Kind == DateTimeKind.Utc)
		{
			timeZoneInfo = Utc;
		}
		if (dateTime.Kind == DateTimeKind.Local)
		{
			timeZoneInfo = Local;
		}
		bool isDST2;
		TimeSpan utcOffsetHelper = GetUtcOffsetHelper(dateTime, timeZoneInfo, out isDST2);
		if (timeZoneInfo == this)
		{
			isDST = isDST2;
			return utcOffsetHelper;
		}
		if (!TryAddTicks(dateTime, -utcOffsetHelper.Ticks, out var result, DateTimeKind.Utc))
		{
			return BaseUtcOffset;
		}
		return GetUtcOffsetHelper(result, this, out isDST);
	}

	private static TimeSpan GetUtcOffsetHelper(DateTime dateTime, TimeZoneInfo tz, out bool isDST)
	{
		if (dateTime.Kind == DateTimeKind.Local && tz != Local)
		{
			throw new Exception();
		}
		isDST = false;
		if (tz == Utc)
		{
			return TimeSpan.Zero;
		}
		if (tz.TryGetTransitionOffset(dateTime, out var offset, out isDST))
		{
			return offset;
		}
		if (dateTime.Kind == DateTimeKind.Utc)
		{
			AdjustmentRule applicableRule = tz.GetApplicableRule(dateTime);
			if (applicableRule != null && tz.IsInDST(applicableRule, dateTime))
			{
				isDST = true;
				return tz.BaseUtcOffset + applicableRule.DaylightDelta;
			}
			return tz.BaseUtcOffset;
		}
		if (!TryAddTicks(dateTime, -tz.BaseUtcOffset.Ticks, out var result, DateTimeKind.Utc))
		{
			return tz.BaseUtcOffset;
		}
		AdjustmentRule applicableRule2 = tz.GetApplicableRule(result);
		DateTime result2 = DateTime.MinValue;
		if (applicableRule2 != null && !TryAddTicks(result, -applicableRule2.DaylightDelta.Ticks, out result2, DateTimeKind.Utc))
		{
			return tz.BaseUtcOffset;
		}
		if (applicableRule2 != null && tz.IsInDST(applicableRule2, dateTime))
		{
			isDST = true;
			if (tz.IsInDST(applicableRule2, result2))
			{
				return tz.BaseUtcOffset + applicableRule2.DaylightDelta;
			}
			return tz.BaseUtcOffset;
		}
		return tz.BaseUtcOffset;
	}

	public bool HasSameRules(TimeZoneInfo other)
	{
		if (other == null)
		{
			throw new ArgumentNullException("other");
		}
		if (adjustmentRules == null != (other.adjustmentRules == null))
		{
			return false;
		}
		if (adjustmentRules == null)
		{
			return true;
		}
		if (BaseUtcOffset != other.BaseUtcOffset)
		{
			return false;
		}
		if (adjustmentRules.Length != other.adjustmentRules.Length)
		{
			return false;
		}
		for (int i = 0; i < adjustmentRules.Length; i++)
		{
			if (!adjustmentRules[i].Equals(other.adjustmentRules[i]))
			{
				return false;
			}
		}
		return true;
	}

	public bool IsAmbiguousTime(DateTime dateTime)
	{
		if (dateTime.Kind == DateTimeKind.Local && IsInvalidTime(dateTime))
		{
			throw new ArgumentException("Kind is Local and time is Invalid");
		}
		if (this == Utc)
		{
			return false;
		}
		if (dateTime.Kind == DateTimeKind.Utc)
		{
			dateTime = ConvertTimeFromUtc(dateTime);
		}
		if (dateTime.Kind == DateTimeKind.Local && this != Local)
		{
			dateTime = ConvertTime(dateTime, Local, this);
		}
		AdjustmentRule applicableRule = GetApplicableRule(dateTime);
		if (applicableRule != null)
		{
			DateTime dateTime2 = TransitionPoint(applicableRule.DaylightTransitionEnd, dateTime.Year);
			if (dateTime > dateTime2 - applicableRule.DaylightDelta && dateTime <= dateTime2)
			{
				return true;
			}
		}
		return false;
	}

	public bool IsAmbiguousTime(DateTimeOffset dateTimeOffset)
	{
		throw new NotImplementedException();
	}

	private bool IsInDST(AdjustmentRule rule, DateTime dateTime)
	{
		if (IsInDSTForYear(rule, dateTime, dateTime.Year))
		{
			return true;
		}
		if (dateTime.Year > 1)
		{
			return IsInDSTForYear(rule, dateTime, dateTime.Year - 1);
		}
		return false;
	}

	private bool IsInDSTForYear(AdjustmentRule rule, DateTime dateTime, int year)
	{
		DateTime dateTime2 = TransitionPoint(rule.DaylightTransitionStart, year);
		DateTime dateTime3 = TransitionPoint(rule.DaylightTransitionEnd, year + ((rule.DaylightTransitionStart.Month >= rule.DaylightTransitionEnd.Month) ? 1 : 0));
		if (dateTime.Kind == DateTimeKind.Utc)
		{
			dateTime2 -= BaseUtcOffset;
			dateTime3 -= BaseUtcOffset + rule.DaylightDelta;
		}
		if (dateTime >= dateTime2)
		{
			return dateTime < dateTime3;
		}
		return false;
	}

	public bool IsDaylightSavingTime(DateTime dateTime)
	{
		if (dateTime.Kind == DateTimeKind.Local && IsInvalidTime(dateTime))
		{
			throw new ArgumentException("dateTime is invalid and Kind is Local");
		}
		if (this == Utc)
		{
			return false;
		}
		if (!SupportsDaylightSavingTime)
		{
			return false;
		}
		GetUtcOffset(dateTime, out var isDST);
		return isDST;
	}

	internal bool IsDaylightSavingTime(DateTime dateTime, TimeZoneInfoOptions flags)
	{
		return IsDaylightSavingTime(dateTime);
	}

	public bool IsDaylightSavingTime(DateTimeOffset dateTimeOffset)
	{
		return IsDaylightSavingTime(dateTimeOffset.DateTime);
	}

	internal DaylightTime GetDaylightChanges(int year)
	{
		DateTime result = DateTime.MinValue;
		DateTime minValue = DateTime.MinValue;
		TimeSpan delta = default(TimeSpan);
		if (transitions != null)
		{
			minValue = DateTime.MaxValue;
			for (int num = transitions.Count - 1; num >= 0; num--)
			{
				KeyValuePair<DateTime, TimeType> keyValuePair = transitions[num];
				DateTime key = keyValuePair.Key;
				TimeType value = keyValuePair.Value;
				if (key.Year <= year)
				{
					if (key.Year < year)
					{
						break;
					}
					if (value.IsDst)
					{
						delta = new TimeSpan(0, 0, value.Offset) - BaseUtcOffset;
						result = key;
					}
					else
					{
						minValue = key;
					}
				}
			}
			if (!TryAddTicks(result, BaseUtcOffset.Ticks, out result))
			{
				result = DateTime.MinValue;
			}
			if (!TryAddTicks(minValue, BaseUtcOffset.Ticks + delta.Ticks, out minValue))
			{
				minValue = DateTime.MinValue;
			}
		}
		else
		{
			AdjustmentRule adjustmentRule = null;
			AdjustmentRule adjustmentRule2 = null;
			AdjustmentRule[] array = GetAdjustmentRules();
			foreach (AdjustmentRule adjustmentRule3 in array)
			{
				if (adjustmentRule3.DateStart.Year <= year && adjustmentRule3.DateEnd.Year >= year)
				{
					if (adjustmentRule3.DateStart.Year <= year && (adjustmentRule == null || adjustmentRule3.DateStart.Year > adjustmentRule.DateStart.Year))
					{
						adjustmentRule = adjustmentRule3;
					}
					if (adjustmentRule3.DateEnd.Year >= year && (adjustmentRule2 == null || adjustmentRule3.DateEnd.Year < adjustmentRule2.DateEnd.Year))
					{
						adjustmentRule2 = adjustmentRule3;
					}
				}
			}
			if (adjustmentRule == null || adjustmentRule2 == null)
			{
				return new DaylightTime(default(DateTime), default(DateTime), default(TimeSpan));
			}
			result = TransitionPoint(adjustmentRule.DaylightTransitionStart, year);
			minValue = TransitionPoint(adjustmentRule2.DaylightTransitionEnd, year);
			delta = adjustmentRule.DaylightDelta;
		}
		if (result == DateTime.MinValue || minValue == DateTime.MinValue)
		{
			return new DaylightTime(default(DateTime), default(DateTime), default(TimeSpan));
		}
		return new DaylightTime(result, minValue, delta);
	}

	public bool IsInvalidTime(DateTime dateTime)
	{
		if (dateTime.Kind == DateTimeKind.Utc)
		{
			return false;
		}
		if (dateTime.Kind == DateTimeKind.Local && this != Local)
		{
			return false;
		}
		AdjustmentRule applicableRule = GetApplicableRule(dateTime);
		if (applicableRule != null)
		{
			DateTime dateTime2 = TransitionPoint(applicableRule.DaylightTransitionStart, dateTime.Year);
			if (dateTime >= dateTime2 && dateTime < dateTime2 + applicableRule.DaylightDelta)
			{
				return true;
			}
		}
		return false;
	}

	void IDeserializationCallback.OnDeserialization(object sender)
	{
		try
		{
			Validate(id, baseUtcOffset, adjustmentRules);
		}
		catch (ArgumentException innerException)
		{
			throw new SerializationException("invalid serialization data", innerException);
		}
	}

	private static void Validate(string id, TimeSpan baseUtcOffset, AdjustmentRule[] adjustmentRules)
	{
		if (id == null)
		{
			throw new ArgumentNullException("id");
		}
		if (id == string.Empty)
		{
			throw new ArgumentException("id parameter is an empty string");
		}
		if (baseUtcOffset.Ticks % 600000000 != 0L)
		{
			throw new ArgumentException("baseUtcOffset parameter does not represent a whole number of minutes");
		}
		if (baseUtcOffset > new TimeSpan(14, 0, 0) || baseUtcOffset < new TimeSpan(-14, 0, 0))
		{
			throw new ArgumentOutOfRangeException("baseUtcOffset parameter is greater than 14 hours or less than -14 hours");
		}
		if (adjustmentRules == null || adjustmentRules.Length == 0)
		{
			return;
		}
		AdjustmentRule adjustmentRule = null;
		foreach (AdjustmentRule adjustmentRule2 in adjustmentRules)
		{
			if (adjustmentRule2 == null)
			{
				throw new InvalidTimeZoneException("one or more elements in adjustmentRules are null");
			}
			if (baseUtcOffset + adjustmentRule2.DaylightDelta < new TimeSpan(-14, 0, 0) || baseUtcOffset + adjustmentRule2.DaylightDelta > new TimeSpan(14, 0, 0))
			{
				throw new InvalidTimeZoneException("Sum of baseUtcOffset and DaylightDelta of one or more object in adjustmentRules array is greater than 14 or less than -14 hours;");
			}
			if (adjustmentRule != null && adjustmentRule.DateStart > adjustmentRule2.DateStart)
			{
				throw new InvalidTimeZoneException("adjustment rules specified in adjustmentRules parameter are not in chronological order");
			}
			if (adjustmentRule != null && adjustmentRule.DateEnd > adjustmentRule2.DateStart)
			{
				throw new InvalidTimeZoneException("some adjustment rules in the adjustmentRules parameter overlap");
			}
			if (adjustmentRule != null && adjustmentRule.DateEnd == adjustmentRule2.DateStart)
			{
				throw new InvalidTimeZoneException("a date can have multiple adjustment rules applied to it");
			}
			adjustmentRule = adjustmentRule2;
		}
	}

	public override string ToString()
	{
		return DisplayName;
	}

	private TimeZoneInfo(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		id = (string)info.GetValue("Id", typeof(string));
		displayName = (string)info.GetValue("DisplayName", typeof(string));
		standardDisplayName = (string)info.GetValue("StandardName", typeof(string));
		daylightDisplayName = (string)info.GetValue("DaylightName", typeof(string));
		baseUtcOffset = (TimeSpan)info.GetValue("BaseUtcOffset", typeof(TimeSpan));
		adjustmentRules = (AdjustmentRule[])info.GetValue("AdjustmentRules", typeof(AdjustmentRule[]));
		supportsDaylightSavingTime = (bool)info.GetValue("SupportsDaylightSavingTime", typeof(bool));
	}

	private TimeZoneInfo(string id, TimeSpan baseUtcOffset, string displayName, string standardDisplayName, string daylightDisplayName, AdjustmentRule[] adjustmentRules, bool disableDaylightSavingTime)
	{
		if (id == null)
		{
			throw new ArgumentNullException("id");
		}
		if (id == string.Empty)
		{
			throw new ArgumentException("id parameter is an empty string");
		}
		if (baseUtcOffset.Ticks % 600000000 != 0L)
		{
			throw new ArgumentException("baseUtcOffset parameter does not represent a whole number of minutes");
		}
		if (baseUtcOffset > new TimeSpan(14, 0, 0) || baseUtcOffset < new TimeSpan(-14, 0, 0))
		{
			throw new ArgumentOutOfRangeException("baseUtcOffset parameter is greater than 14 hours or less than -14 hours");
		}
		bool flag = !disableDaylightSavingTime;
		if (adjustmentRules != null && adjustmentRules.Length != 0)
		{
			AdjustmentRule adjustmentRule = null;
			foreach (AdjustmentRule adjustmentRule2 in adjustmentRules)
			{
				if (adjustmentRule2 == null)
				{
					throw new InvalidTimeZoneException("one or more elements in adjustmentRules are null");
				}
				if (baseUtcOffset + adjustmentRule2.DaylightDelta < new TimeSpan(-14, 0, 0) || baseUtcOffset + adjustmentRule2.DaylightDelta > new TimeSpan(14, 0, 0))
				{
					throw new InvalidTimeZoneException("Sum of baseUtcOffset and DaylightDelta of one or more object in adjustmentRules array is greater than 14 or less than -14 hours;");
				}
				if (adjustmentRule != null && adjustmentRule.DateStart > adjustmentRule2.DateStart)
				{
					throw new InvalidTimeZoneException("adjustment rules specified in adjustmentRules parameter are not in chronological order");
				}
				if (adjustmentRule != null && adjustmentRule.DateEnd > adjustmentRule2.DateStart)
				{
					throw new InvalidTimeZoneException("some adjustment rules in the adjustmentRules parameter overlap");
				}
				if (adjustmentRule != null && adjustmentRule.DateEnd == adjustmentRule2.DateStart)
				{
					throw new InvalidTimeZoneException("a date can have multiple adjustment rules applied to it");
				}
				adjustmentRule = adjustmentRule2;
			}
		}
		else
		{
			flag = false;
		}
		this.id = id;
		this.baseUtcOffset = baseUtcOffset;
		this.displayName = displayName ?? id;
		this.standardDisplayName = standardDisplayName ?? id;
		this.daylightDisplayName = daylightDisplayName;
		supportsDaylightSavingTime = flag;
		this.adjustmentRules = adjustmentRules;
	}

	private AdjustmentRule GetApplicableRule(DateTime dateTime)
	{
		DateTime result = dateTime;
		if (dateTime.Kind == DateTimeKind.Local && this != Local)
		{
			if (!TryAddTicks(result.ToUniversalTime(), BaseUtcOffset.Ticks, out result))
			{
				return null;
			}
		}
		else if (dateTime.Kind == DateTimeKind.Utc && this != Utc && !TryAddTicks(result, BaseUtcOffset.Ticks, out result))
		{
			return null;
		}
		result = result.Date;
		if (adjustmentRules != null)
		{
			AdjustmentRule[] array = adjustmentRules;
			foreach (AdjustmentRule adjustmentRule in array)
			{
				if (adjustmentRule.DateStart > result)
				{
					return null;
				}
				if (!(adjustmentRule.DateEnd < result))
				{
					return adjustmentRule;
				}
			}
		}
		return null;
	}

	private bool TryGetTransitionOffset(DateTime dateTime, out TimeSpan offset, out bool isDst)
	{
		offset = BaseUtcOffset;
		isDst = false;
		if (transitions == null)
		{
			return false;
		}
		DateTime result = dateTime;
		if (dateTime.Kind == DateTimeKind.Local && this != Local && !TryAddTicks(result.ToUniversalTime(), BaseUtcOffset.Ticks, out result, DateTimeKind.Utc))
		{
			return false;
		}
		if (dateTime.Kind != DateTimeKind.Utc && !TryAddTicks(result, -BaseUtcOffset.Ticks, out result, DateTimeKind.Utc))
		{
			return false;
		}
		AdjustmentRule applicableRule = GetApplicableRule(result);
		if (applicableRule != null)
		{
			DateTime dateTime2 = TransitionPoint(applicableRule.DaylightTransitionStart, result.Year);
			DateTime dateTime3 = TransitionPoint(applicableRule.DaylightTransitionEnd, result.Year);
			if (result >= dateTime2 && result <= dateTime3)
			{
				offset = baseUtcOffset + applicableRule.DaylightDelta;
				isDst = true;
				return true;
			}
		}
		return false;
	}

	private static DateTime TransitionPoint(TransitionTime transition, int year)
	{
		if (transition.IsFixedDateRule)
		{
			return new DateTime(year, transition.Month, transition.Day) + transition.TimeOfDay.TimeOfDay;
		}
		DayOfWeek dayOfWeek = new DateTime(year, transition.Month, 1).DayOfWeek;
		int num = 1 + (transition.Week - 1) * 7 + (transition.DayOfWeek - dayOfWeek + 7) % 7;
		if (num > DateTime.DaysInMonth(year, transition.Month))
		{
			num -= 7;
		}
		if (num < 1)
		{
			num += 7;
		}
		return new DateTime(year, transition.Month, num) + transition.TimeOfDay.TimeOfDay;
	}

	private static AdjustmentRule[] ValidateRules(List<AdjustmentRule> adjustmentRules)
	{
		if (adjustmentRules == null || adjustmentRules.Count == 0)
		{
			return null;
		}
		AdjustmentRule adjustmentRule = null;
		AdjustmentRule[] array = adjustmentRules.ToArray();
		foreach (AdjustmentRule adjustmentRule2 in array)
		{
			if (adjustmentRule != null && adjustmentRule.DateEnd > adjustmentRule2.DateStart)
			{
				adjustmentRules.Remove(adjustmentRule2);
			}
			adjustmentRule = adjustmentRule2;
		}
		return adjustmentRules.ToArray();
	}

	private static TimeZoneInfo BuildFromStream(string id, Stream stream)
	{
		byte[] buffer = new byte[16384];
		int length = stream.Read(buffer, 0, 16384);
		if (!ValidTZFile(buffer, length))
		{
			throw new InvalidTimeZoneException("TZ file too big for the buffer");
		}
		try
		{
			return ParseTZBuffer(id, buffer, length);
		}
		catch (InvalidTimeZoneException)
		{
			throw;
		}
		catch (Exception innerException)
		{
			throw new InvalidTimeZoneException("Time zone information file contains invalid data", innerException);
		}
	}

	private static bool ValidTZFile(byte[] buffer, int length)
	{
		StringBuilder stringBuilder = new StringBuilder();
		for (int i = 0; i < 4; i++)
		{
			stringBuilder.Append((char)buffer[i]);
		}
		if (stringBuilder.ToString() != "TZif")
		{
			return false;
		}
		if (length >= 16384)
		{
			return false;
		}
		return true;
	}

	private static int SwapInt32(int i)
	{
		return ((i >> 24) & 0xFF) | ((i >> 8) & 0xFF00) | ((i << 8) & 0xFF0000) | ((i & 0xFF) << 24);
	}

	private static int ReadBigEndianInt32(byte[] buffer, int start)
	{
		int num = BitConverter.ToInt32(buffer, start);
		if (!BitConverter.IsLittleEndian)
		{
			return num;
		}
		return SwapInt32(num);
	}

	private static TimeZoneInfo ParseTZBuffer(string id, byte[] buffer, int length)
	{
		int num = ReadBigEndianInt32(buffer, 20);
		int num2 = ReadBigEndianInt32(buffer, 24);
		int num3 = ReadBigEndianInt32(buffer, 28);
		int num4 = ReadBigEndianInt32(buffer, 32);
		int num5 = ReadBigEndianInt32(buffer, 36);
		int num6 = ReadBigEndianInt32(buffer, 40);
		if (length < 44 + num4 * 5 + num5 * 6 + num6 + num3 * 8 + num2 + num)
		{
			throw new InvalidTimeZoneException();
		}
		Dictionary<int, string> abbreviations = ParseAbbreviations(buffer, 44 + 4 * num4 + num4 + 6 * num5, num6);
		Dictionary<int, TimeType> dictionary = ParseTimesTypes(buffer, 44 + 4 * num4 + num4, num5, abbreviations);
		List<KeyValuePair<DateTime, TimeType>> list = ParseTransitions(buffer, 44, num4, dictionary);
		if (dictionary.Count == 0)
		{
			throw new InvalidTimeZoneException();
		}
		if (dictionary.Count == 1 && dictionary[0].IsDst)
		{
			throw new InvalidTimeZoneException();
		}
		TimeSpan timeSpan = new TimeSpan(0L);
		TimeSpan timeSpan2 = new TimeSpan(0L);
		string text = null;
		string text2 = null;
		bool flag = false;
		DateTime dateTime = DateTime.MinValue;
		List<AdjustmentRule> list2 = new List<AdjustmentRule>();
		bool flag2 = false;
		for (int i = 0; i < list.Count; i++)
		{
			KeyValuePair<DateTime, TimeType> keyValuePair = list[i];
			DateTime key = keyValuePair.Key;
			TimeType value = keyValuePair.Value;
			if (!value.IsDst)
			{
				if (text != value.Name)
				{
					text = value.Name;
				}
				if (timeSpan.TotalSeconds != (double)value.Offset)
				{
					timeSpan = new TimeSpan(0, 0, value.Offset);
					if (list2.Count > 0)
					{
						flag2 = true;
					}
					list2 = new List<AdjustmentRule>();
					flag = false;
				}
				if (flag)
				{
					dateTime += timeSpan;
					DateTime dateTime2 = key + timeSpan + timeSpan2;
					if (dateTime2.Date == new DateTime(dateTime2.Year, 1, 1) && dateTime2.Year > dateTime.Year)
					{
						dateTime2 -= new TimeSpan(24, 0, 0);
					}
					if (dateTime.AddYears(1) < dateTime2)
					{
						flag2 = true;
					}
					DateTime dateStart = ((dateTime.Month >= 7) ? new DateTime(dateTime.Year, 7, 1) : new DateTime(dateTime.Year, 1, 1));
					DateTime dateEnd = ((dateTime2.Month < 7) ? new DateTime(dateTime2.Year, 6, 30) : new DateTime(dateTime2.Year, 12, 31));
					TransitionTime transitionTime = TransitionTime.CreateFixedDateRule(new DateTime(1, 1, 1) + dateTime.TimeOfDay, dateTime.Month, dateTime.Day);
					TransitionTime transitionTime2 = TransitionTime.CreateFixedDateRule(new DateTime(1, 1, 1) + dateTime2.TimeOfDay, dateTime2.Month, dateTime2.Day);
					if (transitionTime != transitionTime2)
					{
						list2.Add(AdjustmentRule.CreateAdjustmentRule(dateStart, dateEnd, timeSpan2, transitionTime, transitionTime2));
					}
				}
				flag = false;
				continue;
			}
			if (text2 != value.Name)
			{
				text2 = value.Name;
			}
			if (timeSpan2.TotalSeconds != (double)value.Offset - timeSpan.TotalSeconds)
			{
				timeSpan2 = new TimeSpan(0, 0, value.Offset) - timeSpan;
				if (timeSpan2.Ticks % 600000000 != 0L)
				{
					timeSpan2 = TimeSpan.FromMinutes((long)(timeSpan2.TotalMinutes + 0.5));
				}
			}
			dateTime = key;
			flag = true;
		}
		TimeZoneInfo timeZoneInfo;
		if (list2.Count == 0 && !flag2)
		{
			if (text == null)
			{
				TimeType timeType = dictionary[0];
				text = timeType.Name;
				timeSpan = new TimeSpan(0, 0, timeType.Offset);
			}
			timeZoneInfo = CreateCustomTimeZone(id, timeSpan, id, text);
		}
		else
		{
			timeZoneInfo = CreateCustomTimeZone(id, timeSpan, id, text, text2, ValidateRules(list2));
		}
		if (flag2 && list.Count > 0)
		{
			timeZoneInfo.transitions = list;
		}
		timeZoneInfo.supportsDaylightSavingTime = list2.Count > 0;
		return timeZoneInfo;
	}

	private static Dictionary<int, string> ParseAbbreviations(byte[] buffer, int index, int count)
	{
		Dictionary<int, string> dictionary = new Dictionary<int, string>();
		int num = 0;
		StringBuilder stringBuilder = new StringBuilder();
		for (int i = 0; i < count; i++)
		{
			char c = (char)buffer[index + i];
			if (c != 0)
			{
				stringBuilder.Append(c);
				continue;
			}
			dictionary.Add(num, stringBuilder.ToString());
			for (int j = 1; j <= stringBuilder.Length; j++)
			{
				dictionary.Add(num + j, stringBuilder.ToString(j, stringBuilder.Length - j));
			}
			num = i + 1;
			stringBuilder = new StringBuilder();
		}
		return dictionary;
	}

	private static Dictionary<int, TimeType> ParseTimesTypes(byte[] buffer, int index, int count, Dictionary<int, string> abbreviations)
	{
		Dictionary<int, TimeType> dictionary = new Dictionary<int, TimeType>(count);
		for (int i = 0; i < count; i++)
		{
			int num = ReadBigEndianInt32(buffer, index + 6 * i);
			num = num / 60 * 60;
			byte b = buffer[index + 6 * i + 4];
			byte key = buffer[index + 6 * i + 5];
			dictionary.Add(i, new TimeType(num, b != 0, abbreviations[key]));
		}
		return dictionary;
	}

	private static List<KeyValuePair<DateTime, TimeType>> ParseTransitions(byte[] buffer, int index, int count, Dictionary<int, TimeType> time_types)
	{
		List<KeyValuePair<DateTime, TimeType>> list = new List<KeyValuePair<DateTime, TimeType>>(count);
		for (int i = 0; i < count; i++)
		{
			DateTime key = DateTimeFromUnixTime(ReadBigEndianInt32(buffer, index + 4 * i));
			byte key2 = buffer[index + 4 * count + i];
			list.Add(new KeyValuePair<DateTime, TimeType>(key, time_types[key2]));
		}
		return list;
	}

	private static DateTime DateTimeFromUnixTime(long unix_time)
	{
		return new DateTime(1970, 1, 1).AddSeconds(unix_time);
	}

	internal static TimeSpan GetLocalUtcOffset(DateTime dateTime, TimeZoneInfoOptions flags)
	{
		bool isDST;
		return Local.GetUtcOffset(dateTime, out isDST);
	}

	internal TimeSpan GetUtcOffset(DateTime dateTime, TimeZoneInfoOptions flags)
	{
		bool isDST;
		return GetUtcOffset(dateTime, out isDST);
	}

	internal static TimeSpan GetUtcOffsetFromUtc(DateTime time, TimeZoneInfo zone, out bool isDaylightSavings, out bool isAmbiguousLocalDst)
	{
		isDaylightSavings = false;
		isAmbiguousLocalDst = false;
		_ = zone.BaseUtcOffset;
		if (zone.IsAmbiguousTime(time))
		{
			isAmbiguousLocalDst = true;
		}
		return zone.GetUtcOffset(time, out isDaylightSavings);
	}

	internal TimeZoneInfo()
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
