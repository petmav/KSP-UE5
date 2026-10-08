using System.Runtime.InteropServices;

namespace System.Resources;

[ComVisible(true)]
[AttributeUsage(AttributeTargets.Assembly, AllowMultiple = false)]
public sealed class SatelliteContractVersionAttribute : Attribute
{
	private string _version;

	public string Version => _version;

	public SatelliteContractVersionAttribute(string version)
	{
		if (version == null)
		{
			throw new ArgumentNullException("version");
		}
		_version = version;
	}
}
