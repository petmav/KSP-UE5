using System;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI
{
	public class ButtonSpritesMgr : MonoBehaviour
	{
		[Serializable]
		public class ButtonSprites
		{
			[SerializeField]
			private Sprite[] sprites;

			public Sprite[] Sprites
			{
				get
				{
					return sprites;
				}
				set
				{
					sprites = value;
				}
			}

			public Sprite Normal => sprites[0];

			public Sprite Hover => sprites[1];

			public Sprite Active => sprites[2];

			public Sprite Disabled => sprites[3];
		}

		[SerializeField]
		private Image btnImg;

		[SerializeField]
		private Button btnCtrl;

		public void SetSpriteSet(ButtonSprites set)
		{
			btnImg.sprite = set.Normal;
			SpriteState spriteState = default(SpriteState);
			spriteState.highlightedSprite = set.Hover;
			spriteState.pressedSprite = set.Active;
			spriteState.disabledSprite = set.Disabled;
			btnCtrl.spriteState = spriteState;
		}
	}
}
