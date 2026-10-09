using UnityEngine;

public class UIPartActionControllerSpawner : MonoBehaviour
{
	public UIPartActionController controllerPrefab;

	private void Start()
	{
		Object.Instantiate(controllerPrefab);
	}
}
