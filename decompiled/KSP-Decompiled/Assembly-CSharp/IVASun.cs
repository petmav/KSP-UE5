using UnityEngine;

public class IVASun : MonoBehaviour
{
	public Transform sunT;

	private Light ivaLight;

	private Light lclLight;

	public float intensityScalar = 0.9f;

	private void Start()
	{
		ivaLight = GetComponent<Light>();
		if (sunT == null)
		{
			sunT = Sun.Instance.transform;
		}
		if (sunT != null)
		{
			lclLight = sunT.GetComponent<Light>();
		}
	}

	private void LateUpdate()
	{
		if (!(FlightGlobals.ActiveVessel == null))
		{
			base.transform.rotation = InternalSpace.WorldToInternal(sunT.rotation);
			if ((bool)ivaLight && (bool)lclLight)
			{
				ivaLight.intensity = lclLight.intensity * intensityScalar;
			}
		}
	}
}
