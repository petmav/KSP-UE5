using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;

namespace System;

[Serializable]
[ComVisible(true)]
public sealed class OperatingSystem : ICloneable, ISerializable
{
	private PlatformID _platform;

	private Version _version;

	private string _servicePack = string.Empty;

	public PlatformID Platform => _platform;

	public Version Version => _version;

	public string ServicePack => _servicePack;

	public string VersionString => ToString();

	public OperatingSystem(PlatformID platform, Version version)
	{
		if (version == null)
		{
			throw new ArgumentNullException("version");
		}
		_platform = platform;
		_version = version;
		if (platform == PlatformID.Win32NT && version.Revision != 0)
		{
			_servicePack = "Service Pack " + (version.Revision >> 16);
		}
	}

	private OperatingSystem(SerializationInfo information, StreamingContext context)
	{
		_platform = (PlatformID)information.GetValue("_platform", typeof(PlatformID));
		_version = (Version)information.GetValue("_version", typeof(Version));
		_servicePack = information.GetString("_servicePack");
	}

	public object Clone()
	{
		return new OperatingSystem(_platform, _version);
	}

	[SecurityCritical]
	public void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		info.AddValue("_platform", _platform);
		info.AddValue("_version", _version);
		info.AddValue("_servicePack", _servicePack);
	}

	public override string ToString()
	{
		string text;
		switch ((int)_platform)
		{
		case 2:
			text = "Microsoft Windows NT";
			break;
		case 0:
			text = "Microsoft Win32S";
			break;
		case 1:
			text = "Microsoft Windows 98";
			break;
		case 3:
			text = "Microsoft Windows CE";
			break;
		case 4:
		case 128:
			text = "Unix";
			break;
		case 5:
			text = "XBox";
			break;
		case 6:
			text = "OSX";
			break;
		default:
			text = Locale.GetText("<unknown>");
			break;
		}
		string text2 = "";
		if (ServicePack != string.Empty)
		{
			text2 = " " + ServicePack;
		}
		return text + " " + _version.ToString() + text2;
	}
}
