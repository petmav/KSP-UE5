using UnityEngine;

namespace EditorGizmos
{
	public class GizmoOffsetHandle : GizmoHandle
	{
		[SerializeField]
		private GizmoOffset host;

		[SerializeField]
		private Transform stretchSection;

		private Vector3 handlePos0;

		private Vector3 handleAxis0;

		private Camera refCamera;

		private Transform trf;

		private Vector2 mousePosAtGrabTime;

		private Vector2 mouseScreenAxis;

		private Vector2 screenAxis;

		private float pixel2WorldFactor;

		private float dOffset;

		private Callback<GizmoOffsetHandle, Vector3> onDragStart;

		private Callback<GizmoOffsetHandle, Vector3, float> onDrag;

		private Callback<GizmoOffsetHandle, Vector3, float> onDragEnded;

		public void Setup(GizmoOffset host, Callback<GizmoOffsetHandle, Vector3> onHandleDragStart, Callback<GizmoOffsetHandle, Vector3, float> onHandleDrag, Callback<GizmoOffsetHandle, Vector3, float> onHandleDragEnd, Camera referenceCamera)
		{
			BaseSetup();
			this.host = host;
			refCamera = referenceCamera;
			trf = base.transform;
			stretchSection.GetComponent<Renderer>().material = primaryRenderer.material;
			onDragStart = onHandleDragStart;
			onDrag = onHandleDrag;
			onDragEnded = onHandleDragEnd;
		}

		protected override bool CanHover()
		{
			return !host.IsDragging;
		}

		protected override void On_MouseEnter()
		{
		}

		protected override void On_MouseDown()
		{
			handlePos0 = trf.position;
			handleAxis0 = handlePos0 - host.transform.position;
			mousePosAtGrabTime = Input.mousePosition;
			screenAxis = refCamera.WorldToScreenPoint(trf.position) - refCamera.WorldToScreenPoint(host.transform.position);
			pixel2WorldFactor = handleAxis0.magnitude / screenAxis.magnitude;
			onDragStart(this, handleAxis0.normalized);
		}

		protected override void On_MouseDrag()
		{
			screenAxis = refCamera.WorldToScreenPoint(trf.position) - refCamera.WorldToScreenPoint(host.transform.position);
			mouseScreenAxis = (Vector2)Input.mousePosition - mousePosAtGrabTime;
			dOffset = Vector2.Dot(mouseScreenAxis, screenAxis.normalized) * pixel2WorldFactor;
			if (host.useGrid)
			{
				dOffset -= dOffset % host.SnapInterval;
			}
			onDrag(this, handleAxis0.normalized, dOffset);
		}

		protected override void On_MouseUp()
		{
			onDragEnded(this, handleAxis0.normalized, dOffset);
		}

		protected override void On_MouseExit()
		{
		}

		public bool IsFacingDirection(Vector3 direction)
		{
			return Vector3.Dot(trf.position - host.transform.position, direction) > 0f;
		}
	}
}
