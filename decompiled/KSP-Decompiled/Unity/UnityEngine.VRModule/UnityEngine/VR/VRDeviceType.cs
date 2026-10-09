using System;
using System.ComponentModel;

namespace UnityEngine.VR;

[Obsolete("VRDeviceType is deprecated. Use XRSettings.supportedDevices instead.", true)]
public enum VRDeviceType
{
	[Obsolete("Enum member VRDeviceType.Morpheus has been deprecated. Use VRDeviceType.PlayStationVR instead (UnityUpgradable) -> PlayStationVR", true)]
	[EditorBrowsable(EditorBrowsableState.Never)]
	Morpheus = -1,
	None,
	Stereo,
	Split,
	Oculus,
	PlayStationVR,
	Unknown
}
