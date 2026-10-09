using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI
{
	public abstract class AnchoredDialog : MonoBehaviour
	{
		private float dialogWidth;

		private float dialogHeight;

		private float paddingHorz;

		private float paddingVert;

		private float clampLeft;

		private float clampRight;

		private float clampBottom;

		private float clampTop;

		private float wDist;

		public float standOffDistance = 45f;

		public float clampPadding = 10f;

		private float zWorldNear;

		private float zWorldFar = 10000f;

		private float zScreenNear = 2f;

		private float zScreenFar = 890f;

		protected bool useOpacityFade;

		public bool clampedToScreen;

		public float nearFadeStart;

		public float nearFadeEnd;

		public float farFadeStart;

		public float farFadeEnd;

		protected float opacity = 1f;

		protected float lastOpacity;

		protected RectTransform rTrf;

		protected Transform anchor;

		protected Transform trf;

		protected Transform camTrf;

		protected Vector3 wPos;

		protected Vector3 sPos;

		protected bool hover;

		private int framesAtSpawn;

		private Camera refCamera;

		private Camera uiCamera;

		protected AnchoredDialogHost anchorHost;

		protected bool disabled;

		[SerializeField]
		protected Image bgPanel;

		[SerializeField]
		protected TextMeshProUGUI windowTitleField;

		private bool willDestroy;

		protected CanvasGroup canvasGroup;

		protected List<XSelectable> uiControls;

		protected abstract void StartThis();

		protected abstract void OnDestroyThis();

		private void Awake()
		{
			trf = base.transform;
			rTrf = GetComponent<RectTransform>();
			canvasGroup = GetComponent<CanvasGroup>();
			framesAtSpawn = Time.frameCount + 5;
			uiCamera = UIMainCamera.Camera;
			willDestroy = false;
			GameEvents.OnGameSettingsApplied.Add(OnGameSettingsApplied);
		}

		private void Start()
		{
			rTrf.SetParent(MainCanvasUtil.MainCanvasRect, worldPositionStays: false);
			camTrf = Camera.main.transform;
			refCamera = Camera.main;
			CreatePanel();
			StartThis();
		}

		private void LateUpdate()
		{
			if (anchor == null)
			{
				return;
			}
			wPos = anchor.position + getUpAxis() * standOffDistance;
			sPos = refCamera.WorldToViewportPoint(wPos);
			if (clampedToScreen)
			{
				sPos.x = Mathf.Clamp(-0.5f + sPos.x, clampLeft, clampRight) * uiCamera.aspect * uiCamera.orthographicSize * 2f;
				sPos.y = Mathf.Clamp(-0.5f + sPos.y, clampBottom, clampTop) * uiCamera.orthographicSize * 2f;
			}
			else
			{
				sPos.x = (-0.5f + sPos.x) * uiCamera.orthographicSize * 2f * uiCamera.aspect;
				sPos.y = (-0.5f + sPos.y) * uiCamera.orthographicSize * 2f;
			}
			if (sPos.z < 0f)
			{
				if (!disabled)
				{
					Disable();
				}
			}
			else if (disabled && opacity > 0.1f)
			{
				ReEnable();
			}
			if (useOpacityFade)
			{
				wDist = (anchor.position - camTrf.position).magnitude;
				opacity = Mathf.Min(Mathf.InverseLerp(nearFadeStart, nearFadeEnd, wDist), Mathf.InverseLerp(farFadeEnd, farFadeStart, wDist));
				if (opacity != lastOpacity)
				{
					setOpacity(opacity);
					lastOpacity = opacity;
				}
				if (opacity < 0.1f && !disabled)
				{
					Disable();
				}
				if (opacity > 0.1f && disabled && sPos.z > 0f)
				{
					ReEnable();
				}
			}
			if (!disabled)
			{
				sPos.z = Mathf.Lerp(zScreenNear, zScreenFar, (sPos.z - zWorldNear) / (zWorldFar - zWorldNear));
				trf.position = sPos;
				hover = GetHover();
				if (!hover && framesAtSpawn <= Time.frameCount && (Mouse.Left.GetButtonUp() || (Mouse.Right.GetButtonUp() && !Mouse.Right.WasDragging())))
				{
					OnClickOut();
				}
			}
			else
			{
				hover = false;
			}
			OnLateUpdate();
		}

		private bool GetHover()
		{
			bool result = false;
			if (uiControls == null)
			{
				return false;
			}
			int count = uiControls.Count;
			while (count-- > 0)
			{
				if (uiControls[count].Hover)
				{
					result = true;
					break;
				}
			}
			return result;
		}

		private void CreatePanel()
		{
			windowTitleField.text = GetWindowTitle();
			CreateWindowContent();
			AttachEventHooks();
			SetBgPanel(bgPanel);
			OnPanelSetupComplete();
		}

		protected virtual void OnPanelSetupComplete()
		{
		}

		private void AttachEventHooks()
		{
			uiControls = new List<XSelectable>();
			Component[] componentsInChildren = base.gameObject.GetComponentsInChildren<Component>(includeInactive: true);
			List<Component> list = new List<Component>(componentsInChildren.Length);
			int num = componentsInChildren.Length;
			while (num-- > 0)
			{
				Component component = componentsInChildren[num];
				if ((component is IEventSystemHandler || component is ICanvasRaycastFilter) && component.gameObject != base.gameObject)
				{
					list.Add(component);
				}
			}
			int count = list.Count;
			while (count-- > 0)
			{
				XSelectable xSelectable = list[count].gameObject.GetComponent<XSelectable>();
				if (xSelectable == null)
				{
					xSelectable = list[count].gameObject.AddComponent<XSelectable>();
				}
				if (!uiControls.Contains(xSelectable))
				{
					uiControls.Add(xSelectable);
				}
			}
		}

		private void Disable()
		{
			OnClickOut();
			if (!willDestroy)
			{
				anchorHost = new GameObject(base.name + " host").AddComponent<AnchoredDialogHost>();
				anchorHost.transform.position = trf.position;
				anchorHost.host = this;
				base.gameObject.SetActive(value: false);
				anchorHost.OnHostLateUpdate = LateUpdate;
				disabled = true;
			}
		}

		private void ReEnable()
		{
			base.gameObject.SetActive(value: true);
			Object.Destroy(anchorHost.gameObject);
			disabled = false;
		}

		public void Terminate()
		{
			if (willDestroy)
			{
				return;
			}
			willDestroy = true;
			if (disabled)
			{
				if (anchorHost.gameObject != null)
				{
					Object.Destroy(anchorHost.gameObject);
				}
			}
			else if (base.gameObject != null)
			{
				Object.Destroy(base.gameObject);
			}
		}

		protected void SetBgPanel(Graphic g)
		{
			RectTransform rectTransform = g.rectTransform;
			paddingHorz = clampPadding / (float)Screen.width;
			paddingVert = clampPadding / (float)Screen.height;
			dialogWidth = rectTransform.rect.width * UIMasterController.Instance.uiScale / (float)Screen.width;
			dialogHeight = rectTransform.rect.height * UIMasterController.Instance.uiScale / (float)Screen.height;
			clampLeft = -0.5f + dialogWidth * 0.5f;
			clampRight = 0.5f - dialogWidth * 0.5f;
			clampTop = 0.5f - dialogHeight;
			clampBottom = -0.5f;
			clampLeft += paddingHorz;
			clampRight -= paddingHorz;
			clampTop -= paddingVert;
			clampBottom += paddingVert;
		}

		protected abstract void CreateWindowContent();

		protected abstract string GetWindowTitle();

		protected abstract void OnClickOut();

		protected virtual void OnLateUpdate()
		{
		}

		private Vector3 getUpAxis()
		{
			return camTrf.parent.up;
		}

		protected void setOpacity(float value)
		{
			if (canvasGroup != null)
			{
				canvasGroup.alpha = value;
			}
		}

		private void OnDestroy()
		{
			GameEvents.OnGameSettingsApplied.Remove(OnGameSettingsApplied);
			OnDestroyThis();
		}

		private void OnGameSettingsApplied()
		{
			SetBgPanel(bgPanel);
		}
	}
}
