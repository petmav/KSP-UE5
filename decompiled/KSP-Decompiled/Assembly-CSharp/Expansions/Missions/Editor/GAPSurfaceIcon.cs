using KSP.UI.Screens.Mapview;
using UnityEngine;

namespace Expansions.Missions.Editor
{
	public class GAPSurfaceIcon
	{
		private MapNode mapNode;

		private string name;

		private double latitude;

		private double longitude;

		private double altitude;

		private MapNode.TypeData typeData;

		private GAPCelestialBody gapRef;

		private bool visible;

		public GAPSurfaceIcon(string name, ref GAPCelestialBody gapRef, Transform parent, MapNode.TypeData typeData, double latitude, double longitude, double altitude)
		{
			this.name = name;
			this.typeData = typeData;
			this.longitude = longitude - gapRef.CelestialBody.directRotAngle;
			this.latitude = latitude;
			this.altitude = altitude;
			this.gapRef = gapRef;
			visible = true;
			mapNode = CreateMapNode(this.name, 32, parent);
		}

		public void OnUpdate()
		{
			mapNode.NodeUpdate();
		}

		protected MapNode CreateMapNode(string name, int size, Transform parent)
		{
			MapNode obj = MapNode.Create(name, Color.white, size, hoverable: true, pinnable: false, blocksInput: false);
			obj.transform.SetParent(parent);
			obj.OnUpdateVisible += OnUpdateVisibleHover;
			obj.OnUpdatePosition += OnUpdatePosition;
			obj.OnUpdateCaption += OnUpdateCaption;
			obj._001E += OnUpdatePositionToUI;
			obj.SetType(typeData);
			return obj;
		}

		private Vector3d OnUpdatePosition(MapNode mn)
		{
			return gapRef.CelestialBody.GetWorldSurfacePosition(latitude, longitude, altitude);
		}

		private void OnUpdateVisibleHover(MapNode mn, MapNode.IconData iData)
		{
			iData.pixelSize = 32;
			iData.visible = visible;
		}

		private void OnUpdateCaption(MapNode mn, MapNode.CaptionData iData)
		{
			iData.Header = mn.name;
		}

		protected Vector3 OnUpdatePositionToUI(MapNode n, Vector3d worldPos)
		{
			Vector3d vector3d = gapRef.CelestialCamera.cam.WorldToScreenPoint(worldPos);
			RectTransform rectTransform = gapRef.displayImage.rectTransform;
			vector3d.x /= gapRef.DisplayTexture.width;
			vector3d.y /= gapRef.DisplayTexture.height;
			vector3d.z = 0.0;
			vector3d.x = vector3d.x * (double)rectTransform.sizeDelta.x - (double)rectTransform.sizeDelta.x * 0.5;
			vector3d.y = vector3d.y * (double)rectTransform.sizeDelta.y - (double)rectTransform.sizeDelta.y * 0.5;
			n.VisualIconData.visible = true;
			return vector3d;
		}

		public void Display(bool value)
		{
			visible = value;
			mapNode.NodeUpdate();
		}

		public void Destroy()
		{
			mapNode.Terminate();
		}
	}
}
