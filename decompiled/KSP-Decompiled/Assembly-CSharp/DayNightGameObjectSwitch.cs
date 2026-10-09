using System.Collections;
using UnityEngine;

public class DayNightGameObjectSwitch : MonoBehaviour
{
	public GameObject[] objects;

	public double longitude;

	public double latitude;

	public double altitude;

	public CelestialBody body;

	public bool OnDuringNight = true;

	private bool objectState;

	private bool setInitialState;

	private Coroutine coroutine;

	private bool setUpComplete;

	private void Start()
	{
		if (objects == null)
		{
			Object.Destroy(this);
		}
		if (objects.Length == 0)
		{
			Object.Destroy(this);
		}
		GameEvents.onLevelWasLoaded.Add(onLevelWasLoaded);
	}

	private void OnDisable()
	{
		StopAllCoroutines();
		coroutine = null;
		setInitialState = false;
	}

	private void OnEnable()
	{
		setInitialState = false;
	}

	private void OnDestroy()
	{
		GameEvents.onLevelWasLoaded.Remove(onLevelWasLoaded);
	}

	private void onLevelWasLoaded(GameScenes scene)
	{
		StopAllCoroutines();
		coroutine = null;
		setInitialState = false;
		switch (scene)
		{
		case GameScenes.SPACECENTER:
			if (setUpComplete)
			{
				break;
			}
			goto case GameScenes.FLIGHT;
		case GameScenes.FLIGHT:
			Setup();
			break;
		}
	}

	private void Setup()
	{
		body = objects[0].GetComponentInParent<CelestialBody>();
		if (!(body == null))
		{
			body.GetLatLonAlt(objects[0].transform.position, out latitude, out longitude, out altitude);
			if (Sun.Instance == null)
			{
				setObjects(state: true);
			}
			setUpComplete = true;
		}
	}

	private void Update()
	{
		if (HighLogic.fetch != null && (HighLogic.LoadedScene == GameScenes.FLIGHT || HighLogic.LoadedScene == GameScenes.SPACECENTER))
		{
			if (!setUpComplete)
			{
				Setup();
			}
			if (setUpComplete && body != null && coroutine == null && base.isActiveAndEnabled)
			{
				coroutine = StartCoroutine(UpdateObjects());
			}
		}
	}

	private IEnumerator UpdateObjects()
	{
		while ((bool)this)
		{
			double localTimeAtPosition = Sun.Instance.GetLocalTimeAtPosition(latitude, longitude, body);
			if (!(localTimeAtPosition < 0.25) && localTimeAtPosition <= 0.699999988079071)
			{
				setObjects(!OnDuringNight);
			}
			else
			{
				setObjects(OnDuringNight);
			}
			yield return new WaitForSeconds(10f);
		}
	}

	private void setObjects(bool state)
	{
		if (objectState == state && setInitialState)
		{
			return;
		}
		for (int i = 0; i < objects.Length; i++)
		{
			if (objects[i].gameObject != null)
			{
				objects[i].gameObject.SetActive(state);
			}
		}
		objectState = state;
		setInitialState = true;
	}
}
