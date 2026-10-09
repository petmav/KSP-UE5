using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	[RequireComponent(typeof(Button))]
	public class MEPartCategoryButton : MonoBehaviour, IPointerEnterHandler, IEventSystemHandler, IPointerExitHandler
	{
		public Image image;

		public Sprite spriteTrue;

		public Sprite spriteFalse;

		public TextMeshProUGUI textCount;

		public bool state = true;

		[SerializeField]
		public Button button;

		private bool hover;

		private bool hoverOnLastDown;

		private void Awake()
		{
			if (button == null && (bool)GetComponent<Button>())
			{
				button = GetComponent<Button>();
			}
		}

		public void SetState(bool state)
		{
			this.state = state;
			if (state)
			{
				image.sprite = spriteTrue;
			}
			else
			{
				image.sprite = spriteFalse;
			}
		}

		public void SetCount(int count)
		{
			textCount.text = count.ToString();
		}

		public void ShowCount(bool showCount)
		{
			textCount.gameObject.SetActive(showCount);
		}

		public void Lock()
		{
			button.interactable = false;
		}

		public void Unlock()
		{
			button.interactable = true;
		}

		[ContextMenu("UpdateState")]
		private void UpdateState()
		{
			SetState(state);
		}

		private void Update()
		{
			hoverOnLastDown = hover;
			if (hover)
			{
				if (Input.GetMouseButtonDown(0) && hoverOnLastDown)
				{
					SetState(!state);
				}
				if (Input.GetMouseButtonDown(1) && hoverOnLastDown)
				{
					SetState(!state);
				}
			}
		}

		public void OnPointerEnter(PointerEventData eventData)
		{
			hover = true;
		}

		public void OnPointerExit(PointerEventData eventData)
		{
			hover = false;
		}
	}
}
