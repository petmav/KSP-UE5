using System.Collections.Generic;
using Expansions;
using UnityEngine;

[KSPScenario((ScenarioCreationOptions)1066, new GameScenes[] { GameScenes.FLIGHT })]
public class ROCScenario : ScenarioModule
{
	public List<ROC> currentROCs;

	public Dictionary<string, ROC> destroyedROCs = new Dictionary<string, ROC>();

	public static ROCScenario Instance { get; private set; }

	private void Start()
	{
		if (!ExpansionsLoader.IsExpansionInstalled("Serenity"))
		{
			Object.Destroy(this);
			Instance = null;
		}
	}
}
