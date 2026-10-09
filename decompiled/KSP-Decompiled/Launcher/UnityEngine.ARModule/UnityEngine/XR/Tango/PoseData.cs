using UnityEngine.Bindings;
using UnityEngine.Scripting;

namespace UnityEngine.XR.Tango;

[NativeHeader("ARScriptingClasses.h")]
[UsedByNativeCode]
internal struct PoseData
{
	public double orientation_x;

	public double orientation_y;

	public double orientation_z;

	public double orientation_w;

	public double translation_x;

	public double translation_y;

	public double translation_z;

	public PoseStatus statusCode;

	public Quaternion rotation => new Quaternion((float)orientation_x, (float)orientation_y, (float)orientation_z, (float)orientation_w);

	public Vector3 position => new Vector3((float)translation_x, (float)translation_y, (float)translation_z);
}
