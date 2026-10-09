using UnityEngine;

public class EditorBackgroundFlag : MonoBehaviour
{
	public MeshRenderer flagMeshRenderer;

	public SkinnedMeshRenderer skinnedMeshRender;

	private Texture2D texture;

	private void Awake()
	{
		GameEvents.onMissionFlagSelect.Add(updateFlag);
	}

	private void OnDestroy()
	{
		GameEvents.onMissionFlagSelect.Remove(updateFlag);
	}

	private void Start()
	{
		if (HighLogic.LoadedSceneIsGame)
		{
			updateFlag((EditorLogic.FlagURL == string.Empty) ? HighLogic.CurrentGame.flagURL : EditorLogic.FlagURL);
		}
	}

	private void updateFlag(string flagURL)
	{
		texture = GameDatabase.Instance.GetTexture(flagURL, asNormalMap: false);
		if (flagMeshRenderer != null && texture != null)
		{
			flagMeshRenderer.material.mainTexture = texture;
		}
		if (skinnedMeshRender != null && texture != null)
		{
			skinnedMeshRender.material.mainTexture = texture;
		}
	}
}
