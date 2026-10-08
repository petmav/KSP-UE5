using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UI;

namespace Expansions.Missions.Editor
{
	public class SpriteButton : MouseRayEventsHandler
	{
		public SpriteRenderer image;

		public SpriteState buttonStates;

		public UnityEvent onClick;

		protected Sprite normalState;

		private bool m_isInteractable = true;

		private bool canClick;

		public bool isInteractable
		{
			get
			{
				return m_isInteractable;
			}
			set
			{
				m_isInteractable = true;
				image.sprite = (value ? normalState : buttonStates.disabledSprite);
			}
		}

		private void Awake()
		{
			normalState = image.sprite;
		}

		protected override void OnMouseEnter()
		{
			if (m_isInteractable)
			{
				base.OnMouseEnter();
				image.sprite = buttonStates.highlightedSprite;
			}
		}

		protected override void OnMouseExit()
		{
			if (m_isInteractable)
			{
				base.OnMouseExit();
				image.sprite = normalState;
			}
		}

		protected override void OnMouseDown()
		{
			if (m_isInteractable)
			{
				base.OnMouseDown();
				canClick = true;
				image.sprite = buttonStates.pressedSprite;
			}
		}

		protected override void OnMouseUp()
		{
			if (m_isInteractable)
			{
				base.OnMouseUp();
				image.sprite = buttonStates.highlightedSprite;
				if (base.isRayOver && canClick)
				{
					onClick.Invoke();
				}
				canClick = false;
			}
		}
	}
}
