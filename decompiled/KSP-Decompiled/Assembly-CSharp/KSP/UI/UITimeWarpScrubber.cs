using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI
{
	public class UITimeWarpScrubber : MonoBehaviour
	{
		[SerializeField]
		private Slider timeSlider;

		private PointerClickAndHoldHandler pointerClickHandler;

		[SerializeField]
		private float timestepValue = 1000f;

		private void Start()
		{
			TimeWarp.SetRate(0, instant: false);
			timeSlider.value = 0f;
			pointerClickHandler = timeSlider.gameObject.GetComponent<PointerClickAndHoldHandler>();
			pointerClickHandler.onPointerUp.AddListener(OnPointerUp);
			pointerClickHandler.onPointerDownHold.AddListener(OnPointerDown);
		}

		private void OnPointerDown(PointerEventData eventData)
		{
			if (!(Planetarium.fetch == null))
			{
				TimeWarp.SetRate(0, instant: false);
				float value = timeSlider.value;
				double universalTime = Planetarium.GetUniversalTime();
				if (value < 0f && universalTime <= 0.0)
				{
					Planetarium.SetUniversalTime(0.0);
				}
				else
				{
					Planetarium.SetUniversalTime(universalTime + (double)(timestepValue * value));
				}
			}
		}

		private void OnPointerUp(PointerEventData eventData)
		{
			timeSlider.value = 0f;
			TimeWarp.SetRate(0, instant: false);
		}
	}
}
