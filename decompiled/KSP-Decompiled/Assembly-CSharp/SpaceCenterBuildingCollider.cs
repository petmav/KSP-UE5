using UnityEngine;

public class SpaceCenterBuildingCollider : MonoBehaviour
{
	public SpaceCenterBuilding building;

	public bool destroyGameObject;

	private void OnDestroy()
	{
		GameEvents.onGameSceneLoadRequested.Remove(OnGameSceneChange);
	}

	public void Setup(SpaceCenterBuilding bld, bool ownGameObject)
	{
		building = bld;
		destroyGameObject = ownGameObject;
		GameEvents.onGameSceneLoadRequested.Add(OnGameSceneChange);
	}

	private void OnGameSceneChange(GameScenes scene)
	{
		GameEvents.onGameSceneLoadRequested.Remove(OnGameSceneChange);
		if (destroyGameObject && base.gameObject != null)
		{
			Object.Destroy(base.gameObject);
		}
		else
		{
			Object.Destroy(this);
		}
	}

	private void OnMouseOver()
	{
		building.ColliderHover(hover: true);
	}

	private void OnMouseExit()
	{
		building.ColliderHover(hover: false);
	}
}
