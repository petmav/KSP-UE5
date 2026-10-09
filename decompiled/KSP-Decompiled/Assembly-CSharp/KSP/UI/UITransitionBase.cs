using System;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI
{
	public abstract class UITransitionBase : MonoBehaviour
	{
		public enum Method
		{
			Linear,
			Lerp
		}

		public RectTransform panelTransform;

		public Method method = Method.Lerp;

		protected Callback<float, Vector2, Vector2> PrepMethod;

		protected Func<float, Vector2, Vector2, Vector2> TransitionMethod;

		[SerializeField]
		private CanvasGroup ctrlGroup;

		private Vector2 vStep;

		protected void Linear_Pre(float transitionTime, Vector2 vCrr, Vector2 vTgt)
		{
			vStep = (vTgt - vCrr) / transitionTime;
		}

		protected Vector2 Linear_Transition(float t, Vector2 vCrr, Vector2 vTgt)
		{
			return vCrr + vStep * Time.deltaTime;
		}

		protected void Lerp_Pre(float transitionTime, Vector2 vCrr, Vector2 vTgt)
		{
		}

		protected Vector2 Lerp_Transition(float t, Vector2 vCrr, Vector2 vTgt)
		{
			return Vector2.Lerp(vCrr, vTgt, t);
		}

		protected void SetMethod(Method m)
		{
			switch (m)
			{
			default:
				PrepMethod = Lerp_Pre;
				TransitionMethod = Lerp_Transition;
				break;
			case Method.Linear:
				PrepMethod = Linear_Pre;
				TransitionMethod = Linear_Transition;
				break;
			}
		}

		protected void SetInteractable(bool interactable)
		{
			if (ctrlGroup != null)
			{
				ctrlGroup.interactable = interactable;
				return;
			}
			Selectable[] componentsInChildren = panelTransform.GetComponentsInChildren<Selectable>();
			int num = componentsInChildren.Length;
			while (num-- > 0)
			{
				componentsInChildren[num].interactable = interactable;
			}
		}
	}
}
