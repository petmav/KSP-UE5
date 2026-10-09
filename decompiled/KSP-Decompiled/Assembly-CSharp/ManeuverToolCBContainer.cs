using System;
using KSP.Localization;
using KSP.UI;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class ManeuverToolCBContainer : MonoBehaviour
{
	public RawImage planetRawImage;

	[NonSerialized]
	public GameObject planet;

	[NonSerialized]
	public bool selected;

	public UIRadioButton background;

	public LayoutElement layoutElement;

	public PointerEnterExitHandler hoverController;

	[NonSerialized]
	public new string name;

	[NonSerialized]
	public string displayName;

	private float scale_original;

	private bool over;

	private bool isRotating;

	private float rotationStartTime;

	private float rotationTimeOffset;

	private float xOffset;

	[NonSerialized]
	public Camera thumbnailCamera;

	[NonSerialized]
	public RenderTexture thumbnailRenderTexture;

	private float thumbnailCameraSize = 60f;

	public LayerMask thumbnailCameraMask;

	public int thumbnailSize = 128;

	private int renderTextureDepth = 24;

	private void OnDestroy()
	{
		background.onTrue.RemoveListener(OnTrue);
		background.onFalse.RemoveListener(OnFalse);
		hoverController.onPointerEnter.RemoveListener(OnPointerEnter);
		hoverController.onPointerExit.RemoveListener(OnPointerExit);
		UnityEngine.Object.Destroy(base.gameObject);
	}

	private void Update()
	{
		if (isRotating)
		{
			RotatePlanet();
		}
	}

	private void SetLightDirection()
	{
		planet.GetComponent<Renderer>().material.SetVector("_localLightDirection", planet.transform.InverseTransformDirection(Vector3.forward).normalized);
	}

	public void Setup(GameObject parent, string bodyName, string displayName, GameObject planet, float scale, float container_height, float xOffset)
	{
		float magnitude = planet.GetComponent<MeshFilter>().mesh.bounds.size.magnitude;
		this.planet = planet;
		this.xOffset = xOffset;
		base.transform.SetParent(parent.transform);
		base.transform.localScale = Vector3.one;
		SetLightDirection();
		if (planet.name.StartsWith("Sun"))
		{
			planet.GetComponent<Renderer>().material.SetFloat("_rimPower", 30f);
			planet.GetComponent<Renderer>().material.SetFloat("_rimBlend", 1f);
		}
		else
		{
			planet.GetComponent<Renderer>().material.SetFloat("_rimPower", 3f);
			planet.GetComponent<Renderer>().material.SetFloat("_rimBlend", 0.3f);
		}
		name = bodyName;
		this.displayName = Localizer.Format("#autoLOC_7001301", displayName);
		scale_original = 130f / magnitude * scale;
		U5Util.SetLayerRecursive(planet.gameObject, LayerMask.NameToLayer("KerbalInstructors"));
		planet.transform.SetParent(base.gameObject.transform);
		planet.transform.localScale = new Vector3(scale_original, scale_original, scale_original);
		planet.transform.localPosition = new Vector3(xOffset, 0f, 1000f);
		planet.transform.rotation = Quaternion.identity;
		SunCoronas[] componentsInChildren = planet.gameObject.GetComponentsInChildren<SunCoronas>();
		int num = componentsInChildren.Length;
		for (int i = 0; i < num; i++)
		{
			UnityEngine.Object.Destroy(componentsInChildren[i].gameObject);
		}
		layoutElement.preferredHeight = container_height;
		background.onTrue.AddListener(OnTrue);
		background.onFalse.AddListener(OnFalse);
		hoverController.onPointerEnter.AddListener(OnPointerEnter);
		hoverController.onPointerExit.AddListener(OnPointerExit);
		CreateThumbnailCamera(bodyName, base.gameObject.transform, out thumbnailCamera, thumbnailCameraSize, thumbnailCameraMask, out thumbnailRenderTexture, thumbnailSize, renderTextureDepth);
	}

	private void CreateThumbnailCamera(string bodyName, Transform anchor, out Camera camRef, float camSize, LayerMask layerMask, out RenderTexture rtRef, int rtSize, int rtDepth)
	{
		camRef = new GameObject("planetCam_" + bodyName).AddComponent<Camera>();
		camRef.orthographic = true;
		camRef.orthographicSize = camSize;
		camRef.cullingMask = layerMask.value;
		camRef.farClipPlane = 500f;
		camRef.clearFlags = CameraClearFlags.Color;
		camRef.backgroundColor = Color.clear;
		camRef.transform.SetParent(anchor);
		camRef.transform.localPosition = new Vector3(xOffset, 0f, 900f);
		rtRef = new RenderTexture(rtSize, rtSize, rtDepth, RenderTextureFormat.ARGB32);
		camRef.targetTexture = rtRef;
		planetRawImage.texture = rtRef;
	}

	private void OnTrue(PointerEventData data, UIRadioButton.CallType callType)
	{
		selected = true;
		StartRotation();
	}

	private void OnFalse(PointerEventData data, UIRadioButton.CallType callType)
	{
		if (callType == UIRadioButton.CallType.USER)
		{
			selected = false;
			StopRotation();
		}
	}

	private void OnPointerEnter(PointerEventData data)
	{
		if (!over)
		{
			over = true;
			StartRotation();
		}
	}

	private void OnPointerExit(PointerEventData data)
	{
		if (over)
		{
			over = false;
			StopRotation();
		}
	}

	private void StartRotation()
	{
		if (!isRotating)
		{
			rotationStartTime = Time.realtimeSinceStartup;
			isRotating = true;
		}
	}

	private void StopRotation()
	{
		if (isRotating)
		{
			float realtimeSinceStartup = Time.realtimeSinceStartup;
			rotationTimeOffset += realtimeSinceStartup - rotationStartTime;
			rotationStartTime = realtimeSinceStartup;
			RotatePlanet();
			isRotating = false;
		}
	}

	private void RotatePlanet()
	{
		SetLightDirection();
		planet.transform.rotation = Quaternion.identity;
		planet.transform.Rotate(Vector3.up, 75f * (Time.realtimeSinceStartup - rotationStartTime + rotationTimeOffset));
	}

	public void Hide()
	{
		base.gameObject.SetActive(value: false);
	}

	public void Show(Vector2 position, float scale)
	{
		base.transform.localScale = new Vector3(scale, scale, scale);
		base.transform.localPosition = position;
		base.gameObject.SetActive(value: true);
	}
}
