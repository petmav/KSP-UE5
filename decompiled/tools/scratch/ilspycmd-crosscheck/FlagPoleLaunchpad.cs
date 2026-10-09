using UnityEngine;

public class FlagPoleLaunchpad : MonoBehaviour
{
	public Animation flagAnimation;

	public string animationName = "animation";

	public Renderer flagRenderer;

	private Texture2D flagTexture;

	private void Start()
	{
		GameEvents.onFlagSelect.Add(OnFlagSelect);
		GameEvents.onGameSceneLoadRequested.Add(OnSceneChange);
		if (flagAnimation != null)
		{
			flagAnimation.Play(animationName);
		}
		OnSceneChange(GameScenes.LOADING);
	}

	private void OnDestroy()
	{
		GameEvents.onFlagSelect.Remove(OnFlagSelect);
		GameEvents.onGameSceneLoadRequested.Remove(OnSceneChange);
	}

	private void OnSceneChange(GameScenes scene)
	{
		if (HighLogic.CurrentGame != null)
		{
			OnFlagSelect(HighLogic.CurrentGame.flagURL);
		}
	}

	private void OnFlagSelect(string flagName)
	{
		flagTexture = GameDatabase.Instance.GetTexture(flagName, asNormalMap: false);
		flagRenderer.material.mainTexture = flagTexture;
	}
}
