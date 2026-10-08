using UnityEngine;
using UnityEngine.SceneManagement;

namespace TestScripts
{
	public class FacilitySceneryLoader : MonoBehaviour
	{
		public EditorFacility facilityScenery;

		private EditorFacility loadedScenery;

		private void Start()
		{
			UnloadScenery();
			LoadScenery(facilityScenery);
		}

		public void LoadScenery(EditorFacility facility)
		{
			switch (facilityScenery)
			{
			case EditorFacility.VAB:
				SceneManager.LoadScene("VABscenery", LoadSceneMode.Additive);
				break;
			case EditorFacility.SPH:
				SceneManager.LoadScene("SPHscenery", LoadSceneMode.Additive);
				break;
			}
			loadedScenery = facility;
		}

		public void UnloadScenery()
		{
			switch (loadedScenery)
			{
			case EditorFacility.VAB:
				Object.DestroyImmediate(GameObject.Find("VABscenery"));
				break;
			case EditorFacility.SPH:
				Object.DestroyImmediate(GameObject.Find("SPHscenery"));
				break;
			}
			loadedScenery = EditorFacility.None;
		}
	}
}
