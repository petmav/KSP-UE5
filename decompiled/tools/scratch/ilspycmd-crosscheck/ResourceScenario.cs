using UnityEngine;

[KSPScenario((ScenarioCreationOptions)3198, new GameScenes[]
{
	GameScenes.FLIGHT,
	GameScenes.TRACKSTATION,
	GameScenes.SPACECENTER,
	GameScenes.EDITOR,
	GameScenes.MISSIONBUILDER
})]
public class ResourceScenario : ScenarioModule
{
	private ResourceMap map;

	public static ResourceScenario Instance { get; private set; }

	public ResourceGameSettings gameSettings { get; private set; }

	public override void OnAwake()
	{
		Instance = this;
		gameSettings = new ResourceGameSettings();
		map = base.gameObject.AddComponent<ResourceMap>();
	}

	private void OnDestroy()
	{
		if (map != null)
		{
			Object.Destroy(map);
		}
		if (Instance != null && Instance == this)
		{
			Instance = null;
		}
	}

	protected virtual void Start()
	{
		map.ResetCache();
	}

	public override void OnLoad(ConfigNode gameNode)
	{
		base.OnLoad(gameNode);
		gameSettings.Load(gameNode);
		if (ResourceMap.Instance != null)
		{
			ResourceMap.Instance.ResetCache();
		}
	}

	public override void OnSave(ConfigNode gameNode)
	{
		base.OnSave(gameNode);
		gameSettings.Save(gameNode);
	}
}
