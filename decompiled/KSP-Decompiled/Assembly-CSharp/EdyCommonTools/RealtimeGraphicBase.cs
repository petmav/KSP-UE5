using UnityEngine;

namespace EdyCommonTools
{
	public class RealtimeGraphicBase : MonoBehaviour
	{
		public int width = 1200;

		public int height = 512;

		public Rect rect = new Rect(-5f, -2.5f, 10f, 5f);

		public float gridX = 1f;

		public float gridY = 1f;

		public Font font;

		private TextureCanvas m_graphic;

		private GUIStyle m_style = new GUIStyle();

		private string m_text = "";

		private float m_lastGridX = 1f;

		private float m_lastGridY = 1f;

		public TextureCanvas canvas => m_graphic;

		public virtual void OnInitialize()
		{
		}

		public virtual void OnUpdateGraphic()
		{
		}

		public virtual string OnUpdateText()
		{
			return "-";
		}

		private void OnEnable()
		{
			SetupGraphic();
			OnInitialize();
			SetupText();
		}

		private void OnValidate()
		{
			SetupText();
		}

		private void Update()
		{
			if (rect != m_graphic.rect || gridX != m_lastGridX || gridY != m_lastGridY || width != m_graphic.width || height != m_graphic.height)
			{
				m_lastGridX = gridX;
				m_lastGridY = gridY;
				SetupGraphic();
			}
			UpdateGraphic();
		}

		private void SetupGraphic()
		{
			if (m_graphic != null)
			{
				m_graphic.DestroyTexture();
			}
			m_graphic = new TextureCanvas(width, height, rect);
			m_graphic.alpha = 0.5f;
			m_graphic.color = Color.black;
			m_graphic.Clear();
			m_graphic.alpha = 0.65f;
			m_graphic.color = Color.green * 0.25f;
			m_graphic.Grid(gridX, gridY);
			m_graphic.color = Color.white * 0.6f;
			m_graphic.HorizontalLine(0f);
			m_graphic.VerticalLine(0f);
			m_graphic.Save();
		}

		private void SetupText()
		{
			m_style.font = font;
			m_style.fontSize = 10;
			m_style.normal.textColor = Color.white;
		}

		private void UpdateGraphic()
		{
			m_graphic.Restore();
			m_graphic.alpha = 0.65f;
			m_graphic.alphaBlend = false;
			OnUpdateGraphic();
			m_text = OnUpdateText();
		}

		private void OnGUI()
		{
			m_graphic.GUIDraw(16, 16);
			GUI.Label(new Rect(24f, 24f, width, height), m_text, m_style);
		}
	}
}
