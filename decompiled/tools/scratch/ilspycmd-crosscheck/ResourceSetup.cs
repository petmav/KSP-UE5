using UnityEngine;

public class ResourceSetup : MonoBehaviour
{
	public class ResourceConfig
	{
		public bool HeatEnabled { get; set; }

		public int ECMinScale { get; set; }

		public int OverlayStyle { get; set; }

		public bool ShowDebugOptions { get; set; }
	}

	private static ResourceSetup instance;

	private static ResourceConfig _resConfig;

	public static ResourceSetup Instance => instance ?? (instance = new GameObject("ResourceSetup").AddComponent<ResourceSetup>());

	public ResourceConfig ResConfig => _resConfig ?? (_resConfig = LoadResourceConfig());

	private void OnDestroy()
	{
		if (instance != null && instance == this)
		{
			instance = null;
		}
	}

	private ResourceConfig LoadResourceConfig()
	{
		if (GameDatabase.Instance.GetConfigNodes("RESOURCE_CONFIGURATION").Length == 0)
		{
			return null;
		}
		return ResourceUtilities.LoadNodeProperties<ResourceConfig>(GameDatabase.Instance.GetConfigNodes("RESOURCE_CONFIGURATION")[0]);
	}
}
