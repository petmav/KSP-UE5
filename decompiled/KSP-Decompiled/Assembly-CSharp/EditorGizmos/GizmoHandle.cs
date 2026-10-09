using UnityEngine;

namespace EditorGizmos
{
	public abstract class GizmoHandle : MonoBehaviour, IMouseEvents
	{
		[SerializeField]
		protected Color normalColor;

		[SerializeField]
		protected Color hoverColor;

		[SerializeField]
		protected Color downColor;

		[SerializeField]
		protected Color disabledColor;

		protected bool hover;

		protected bool drag;

		[SerializeField]
		protected Renderer primaryRenderer;

		[SerializeField]
		protected Renderer highlightRenderer;

		public bool Hover => hover;

		public bool Drag => drag;

		public void OnMouseEnter()
		{
			if (!hover && CanHover())
			{
				hover = true;
				primaryRenderer.material.color = hoverColor;
				On_MouseEnter();
			}
		}

		public void OnMouseDown()
		{
			if (hover)
			{
				drag = true;
				primaryRenderer.material.color = downColor;
				On_MouseDown();
			}
		}

		public void OnMouseDrag()
		{
			if (drag)
			{
				On_MouseDrag();
			}
		}

		public void OnMouseUp()
		{
			if (!hover)
			{
				OnMouseOut();
			}
			else
			{
				primaryRenderer.material.color = hoverColor;
			}
			On_MouseUp();
			drag = false;
		}

		public void OnMouseExit()
		{
			if (!drag && CanHover())
			{
				OnMouseOut();
			}
			On_MouseExit();
			hover = false;
		}

		private void OnMouseOut()
		{
			primaryRenderer.material.color = normalColor;
		}

		public MonoBehaviour GetInstance()
		{
			return this;
		}

		public void SetLock(bool lockSt)
		{
			if (lockSt)
			{
				primaryRenderer.material.color = disabledColor;
				if (highlightRenderer != null)
				{
					highlightRenderer.enabled = false;
				}
			}
			else
			{
				primaryRenderer.material.color = (hover ? hoverColor : normalColor);
				if (highlightRenderer != null)
				{
					highlightRenderer.enabled = true;
				}
			}
		}

		protected void BaseSetup()
		{
			primaryRenderer = GetComponent<Renderer>();
			primaryRenderer.material.color = normalColor;
		}

		protected abstract bool CanHover();

		protected abstract void On_MouseEnter();

		protected abstract void On_MouseDown();

		protected abstract void On_MouseDrag();

		protected abstract void On_MouseUp();

		protected abstract void On_MouseExit();
	}
}
