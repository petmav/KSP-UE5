using System.Collections.Generic;
using System.Globalization;

namespace System.Net.Http.Headers;

internal static class QualityValue
{
	public static double? GetValue(List<NameValueHeaderValue> parameters)
	{
		if (parameters == null)
		{
			return null;
		}
		NameValueHeaderValue nameValueHeaderValue = parameters.Find((NameValueHeaderValue l) => string.Equals(l.Name, "q", StringComparison.OrdinalIgnoreCase));
		if (nameValueHeaderValue == null)
		{
			return null;
		}
		if (!double.TryParse(nameValueHeaderValue.Value, NumberStyles.Number, NumberFormatInfo.InvariantInfo, out var result))
		{
			return null;
		}
		return result;
	}

	public static void SetValue(ref List<NameValueHeaderValue> parameters, double? value)
	{
		if (value < 0.0 || value > 1.0)
		{
			throw new ArgumentOutOfRangeException("Quality");
		}
		if (parameters == null)
		{
			parameters = new List<NameValueHeaderValue>();
		}
		parameters.SetValue("q", (!value.HasValue) ? null : value.Value.ToString(NumberFormatInfo.InvariantInfo));
	}
}
