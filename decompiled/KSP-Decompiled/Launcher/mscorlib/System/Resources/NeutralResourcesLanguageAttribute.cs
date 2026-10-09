using System.Runtime.InteropServices;

namespace System.Resources;

[ComVisible(true)]
[AttributeUsage(AttributeTargets.Assembly, AllowMultiple = false)]
public sealed class NeutralResourcesLanguageAttribute : Attribute
{
	private string _culture;

	private UltimateResourceFallbackLocation _fallbackLoc;

	public string CultureName => _culture;

	public UltimateResourceFallbackLocation Location => _fallbackLoc;

	public NeutralResourcesLanguageAttribute(string cultureName)
	{
		if (cultureName == null)
		{
			throw new ArgumentNullException("cultureName");
		}
		_culture = cultureName;
		_fallbackLoc = UltimateResourceFallbackLocation.MainAssembly;
	}

	public NeutralResourcesLanguageAttribute(string cultureName, UltimateResourceFallbackLocation location)
	{
		if (cultureName == null)
		{
			throw new ArgumentNullException("cultureName");
		}
		if (!Enum.IsDefined(typeof(UltimateResourceFallbackLocation), location))
		{
			throw new ArgumentException(Environment.GetResourceString("The NeutralResourcesLanguageAttribute specifies an invalid or unrecognized ultimate resource fallback location: \"{0}\".", location));
		}
		_culture = cultureName;
		_fallbackLoc = location;
	}
}
