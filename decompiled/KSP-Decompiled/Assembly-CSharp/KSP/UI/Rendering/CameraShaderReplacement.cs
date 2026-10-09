using UnityEngine;

namespace KSP.UI.Rendering
{
	[RequireComponent(typeof(Camera))]
	public class CameraShaderReplacement : MonoBehaviour
	{
		[SerializeField]
		private Shader shader;

		[SerializeField]
		private string replacementTag = string.Empty;

		[SerializeField]
		private Camera cam;

		private void Reset()
		{
			cam = base.gameObject.GetComponent<Camera>();
		}

		private void Start()
		{
			SetShader();
		}

		[ContextMenu("Reset Shader")]
		public void SetShader()
		{
			cam = base.gameObject.GetComponent<Camera>();
			cam.SetReplacementShader(shader, replacementTag);
		}
	}
}
