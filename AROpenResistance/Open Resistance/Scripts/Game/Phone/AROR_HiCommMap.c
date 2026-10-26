//! World-space terrain geometry for HICOMM's map, sampled from the terrain on each client (sea level is 0, as in the
//! HQ rules) and cached for the session. Works for any terrain: extender maps only set the map bounds on
//! AROR_HighCommandComponent. Built in steps (one per frame) so opening the map never stalls: first the land and
//! coastline, then one elevation contour level per step.
class AROR_HiCommTerrain
{
	//! Samples per side; 160 gives 80 m cells on Everon
	static const int RESOLUTION = 160;

	//! Contour levels (m) built after the coastline; every second one is a major contour
	protected static const int CONTOUR_START = 50;
	protected static const int CONTOUR_STEP = 50;
	protected static const int CONTOUR_COUNT = 7;

	static const int LINE_COAST = 0;
	static const int LINE_CONTOUR = 1;
	static const int LINE_CONTOUR_MAJOR = 2;

	protected static ref AROR_HiCommTerrain s_Instance;

	protected vector m_vMin;
	protected vector m_vMax;
	protected float m_fCellX;
	protected float m_fCellZ;
	protected ref array<float> m_aHeights = {};
	protected int m_iStep;

	//! Elevation bands for land shading (m per band) and water/coast band codes
	static const int LAND_BANDS = 8;
	protected static const float LAND_BAND_HEIGHT = 40;
	static const int SEA_SHALLOW = -1;
	static const int SEA_MID = -2;
	static const int SEA_DEEP = -3;
	static const int SHORE = -4;

	//! Shaded terrain: convex polygons as flat world x,z pairs, their band (see CellBand) and bounds (minX, minZ, maxX, maxZ)
	ref array<ref array<float>> m_aLand = {};
	ref array<int> m_aLandBands = {};
	ref array<float> m_aLandBounds = {};

	//! Coastline and contours: polylines as flat world x,z pairs, their kind (LINE_*) and bounds
	ref array<ref array<float>> m_aLines = {};
	ref array<int> m_aLineKinds = {};
	ref array<float> m_aLineBounds = {};

	//------------------------------------------------------------------------------------------------
	//! Cached; starts again only if the bounds change (another world)
	static AROR_HiCommTerrain Get(vector min, vector max)
	{
		if (!s_Instance || s_Instance.m_vMin != min || s_Instance.m_vMax != max)
		{
			s_Instance = new AROR_HiCommTerrain();
			s_Instance.m_vMin = min;
			s_Instance.m_vMax = max;
			s_Instance.m_fCellX = (max[0] - min[0]) / RESOLUTION;
			s_Instance.m_fCellZ = (max[2] - min[2]) / RESOLUTION;
		}

		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	bool IsComplete()
	{
		return m_iStep > CONTOUR_COUNT;
	}

	//------------------------------------------------------------------------------------------------
	//! Builds the next piece; returns true if geometry was added
	bool BuildStep()
	{
		if (IsComplete())
			return false;

		if (m_iStep == 0)
		{
			SampleHeights();
			BuildLand();
			BuildContour(0, LINE_COAST);
		}
		else
		{
			int level = CONTOUR_START + (m_iStep - 1) * CONTOUR_STEP;
			int kind = LINE_CONTOUR;
			if (Math.Mod(level, CONTOUR_STEP * 2) == 0)
				kind = LINE_CONTOUR_MAJOR;

			BuildContour(level, kind);
		}

		m_iStep++;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void SampleHeights()
	{
		m_aHeights.Clear();
		BaseWorld world = GetGame().GetWorld();
		for (int row = 0; row <= RESOLUTION; row++)
		{
			for (int column = 0; column <= RESOLUTION; column++)
			{
				float height = -1;
				if (world)
					height = world.GetSurfaceY(m_vMin[0] + column * m_fCellX, m_vMin[2] + row * m_fCellZ);

				m_aHeights.Insert(height);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected float Height(int column, int row)
	{
		return m_aHeights[row * (RESOLUTION + 1) + column];
	}

	//------------------------------------------------------------------------------------------------
	//! Shading band of a whole cell: land 1-LAND_BANDS by elevation, water SEA_SHALLOW/SEA_MID by depth,
	//! SEA_DEEP (not drawn, the background) or SHORE for cells the coastline crosses
	protected int CellBand(int column, int row)
	{
		int caseIndex = CellCase(column, row, 0);
		if (caseIndex != 0 && caseIndex != 15)
			return SHORE;

		float average = (Height(column, row) + Height(column + 1, row) + Height(column + 1, row + 1) + Height(column, row + 1)) * 0.25;
		if (caseIndex == 15)
		{
			int elevationBand = Math.Floor(average / LAND_BAND_HEIGHT);
			return 1 + Math.ClampInt(elevationBand, 0, LAND_BANDS - 1);
		}

		if (average > -6)
			return SEA_SHALLOW;

		if (average > -25)
			return SEA_MID;

		return SEA_DEEP;
	}

	//------------------------------------------------------------------------------------------------
	//! Cells of one band merge into a rectangle per row run; coastal cells get shallow water plus their exact
	//! (convex) land part, so the fill matches the interpolated coastline
	protected void BuildLand()
	{
		for (int row = 0; row < RESOLUTION; row++)
		{
			int runStart = -1;
			int runBand = SEA_DEEP;
			for (int column = 0; column <= RESOLUTION; column++)
			{
				int band = SEA_DEEP;
				if (column < RESOLUTION)
					band = CellBand(column, row);

				if (runStart >= 0 && band != runBand)
				{
					AddRun(runStart, column, row, runBand);
					runStart = -1;
				}

				if (band == SHORE)
				{
					AddRun(column, column + 1, row, SEA_SHALLOW);
					AddLand(CoastalCellPolygon(column, row), 1);
					continue;
				}

				if (runStart < 0 && band != SEA_DEEP)
				{
					runStart = column;
					runBand = band;
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void AddRun(int startColumn, int endColumn, int row, int band)
	{
		float left = m_vMin[0] + startColumn * m_fCellX;
		float right = m_vMin[0] + endColumn * m_fCellX;
		float bottom = m_vMin[2] + row * m_fCellZ;
		float top = bottom + m_fCellZ;
		AddLand({left, bottom, right, bottom, right, top, left, top}, band);
	}

	//------------------------------------------------------------------------------------------------
	//! Corners bl, br, tr, tl in order: land corners plus the shoreline crossing on every edge that changes
	protected array<float> CoastalCellPolygon(int column, int row)
	{
		array<int> columns = {column, column + 1, column + 1, column};
		array<int> rows = {row, row, row + 1, row + 1};
		array<float> polygon = {};
		for (int corner = 0; corner < 4; corner++)
		{
			int next = Math.Mod(corner + 1, 4);
			float height = Height(columns[corner], rows[corner]);
			float nextHeight = Height(columns[next], rows[next]);
			if (height > 0)
			{
				polygon.Insert(m_vMin[0] + columns[corner] * m_fCellX);
				polygon.Insert(m_vMin[2] + rows[corner] * m_fCellZ);
			}

			if ((height > 0) != (nextHeight > 0))
			{
				float t = height / (height - nextHeight);
				polygon.Insert(m_vMin[0] + Math.Lerp(columns[corner], columns[next], t) * m_fCellX);
				polygon.Insert(m_vMin[2] + Math.Lerp(rows[corner], rows[next], t) * m_fCellZ);
			}
		}

		return polygon;
	}

	//------------------------------------------------------------------------------------------------
	protected void AddLand(array<float> polygon, int band)
	{
		m_aLand.Insert(polygon);
		m_aLandBands.Insert(band);
		AppendBounds(polygon, m_aLandBounds);
	}

	//------------------------------------------------------------------------------------------------
	//! Corner bits: bl 1, br 2, tr 4, tl 8 (above level)
	protected int CellCase(int column, int row, float level)
	{
		int caseIndex;
		if (Height(column, row) > level)
			caseIndex |= 1;

		if (Height(column + 1, row) > level)
			caseIndex |= 2;

		if (Height(column + 1, row + 1) > level)
			caseIndex |= 4;

		if (Height(column, row + 1) > level)
			caseIndex |= 8;

		return caseIndex;
	}

	//------------------------------------------------------------------------------------------------
	// Edges are identified by their first grid point and direction: (point * 2) horizontal, (point * 2 + 1) vertical
	//------------------------------------------------------------------------------------------------
	protected int HorizontalEdge(int column, int row)
	{
		return (row * (RESOLUTION + 1) + column) * 2;
	}

	//------------------------------------------------------------------------------------------------
	protected int VerticalEdge(int column, int row)
	{
		return (row * (RESOLUTION + 1) + column) * 2 + 1;
	}

	//------------------------------------------------------------------------------------------------
	//! Where the level crosses an edge (world x, z), interpolated between its two grid points
	protected void EdgePoint(int edge, float level, out float x, out float z)
	{
		int point = edge / 2;
		int column = Math.Mod(point, RESOLUTION + 1);
		int row = point / (RESOLUTION + 1);
		int otherColumn = column;
		int otherRow = row;
		if (Math.Mod(edge, 2) == 0)
			otherColumn++;
		else
			otherRow++;

		float a = Height(column, row);
		float b = Height(otherColumn, otherRow);
		float t = 0.5;
		if (Math.AbsFloat(a - b) > 0.001)
			t = Math.Clamp((level - a) / (b - a), 0, 1);

		x = m_vMin[0] + Math.Lerp(column, otherColumn, t) * m_fCellX;
		z = m_vMin[2] + Math.Lerp(row, otherRow, t) * m_fCellZ;
	}

	//------------------------------------------------------------------------------------------------
	//! Marching squares at one level, then segments chained into polylines (far fewer draw commands)
	protected void BuildContour(float level, int kind)
	{
		array<int> segmentA = {};
		array<int> segmentB = {};
		for (int row = 0; row < RESOLUTION; row++)
		{
			for (int column = 0; column < RESOLUTION; column++)
			{
				int caseIndex = CellCase(column, row, level);
				if (caseIndex == 0 || caseIndex == 15)
					continue;

				int south = HorizontalEdge(column, row);
				int north = HorizontalEdge(column, row + 1);
				int west = VerticalEdge(column, row);
				int east = VerticalEdge(column + 1, row);

				// A case and its inverse (15 - case) share the same boundary
				int shape = Math.Min(caseIndex, 15 - caseIndex);
				if (shape == 1)
					AddSegment(segmentA, segmentB, west, south);
				else if (shape == 2)
					AddSegment(segmentA, segmentB, south, east);
				else if (shape == 3)
					AddSegment(segmentA, segmentB, west, east);
				else if (shape == 4)
					AddSegment(segmentA, segmentB, east, north);
				else if (shape == 6)
					AddSegment(segmentA, segmentB, south, north);
				else if (shape == 7)
					AddSegment(segmentA, segmentB, west, north);
				else if (caseIndex == 5)
				{
					// Saddles keep the high corners connected (matches the land polygons)
					AddSegment(segmentA, segmentB, west, north);
					AddSegment(segmentA, segmentB, south, east);
				}
				else if (caseIndex == 10)
				{
					AddSegment(segmentA, segmentB, west, south);
					AddSegment(segmentA, segmentB, east, north);
				}
			}
		}

		ChainSegments(segmentA, segmentB, level, kind);
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddSegment(array<int> segmentA, array<int> segmentB, int edgeA, int edgeB)
	{
		segmentA.Insert(edgeA);
		segmentB.Insert(edgeB);
	}

	//------------------------------------------------------------------------------------------------
	protected void ChainSegments(array<int> segmentA, array<int> segmentB, float level, int kind)
	{
		// Every edge touches at most two segments
		map<int, ref array<int>> segmentsByEdge = new map<int, ref array<int>>();
		int count = segmentA.Count();
		for (int i = 0; i < count; i++)
		{
			LinkEdge(segmentsByEdge, segmentA[i], i);
			LinkEdge(segmentsByEdge, segmentB[i], i);
		}

		array<bool> used = {};
		used.Resize(count);

		for (int start = 0; start < count; start++)
		{
			if (used[start])
				continue;

			used[start] = true;
			array<int> edges = {segmentA[start], segmentB[start]};
			ExtendChain(edges, false, segmentsByEdge, segmentA, segmentB, used);
			ExtendChain(edges, true, segmentsByEdge, segmentA, segmentB, used);

			array<float> line = {};
			foreach (int edge : edges)
			{
				float x, z;
				EdgePoint(edge, level, x, z);
				line.Insert(x);
				line.Insert(z);
			}

			m_aLines.Insert(line);
			m_aLineKinds.Insert(kind);
			AppendBounds(line, m_aLineBounds);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void LinkEdge(map<int, ref array<int>> segmentsByEdge, int edge, int segment)
	{
		array<int> segments = segmentsByEdge.Get(edge);
		if (!segments)
		{
			segments = {};
			segmentsByEdge.Set(edge, segments);
		}

		segments.Insert(segment);
	}

	//------------------------------------------------------------------------------------------------
	//! Follows unused segments from the chain's end (or its start, atFront) until the line ends or closes
	protected static void ExtendChain(array<int> edges, bool atFront, map<int, ref array<int>> segmentsByEdge, array<int> segmentA, array<int> segmentB, array<bool> used)
	{
		while (true)
		{
			int current = edges[edges.Count() - 1];
			if (atFront)
				current = edges[0];

			array<int> segments = segmentsByEdge.Get(current);
			int nextSegment = -1;
			if (segments)
			{
				foreach (int segment : segments)
				{
					if (!used[segment])
					{
						nextSegment = segment;
						break;
					}
				}
			}

			if (nextSegment < 0)
				return;

			used[nextSegment] = true;
			int nextEdge = segmentA[nextSegment];
			if (nextEdge == current)
				nextEdge = segmentB[nextSegment];

			if (atFront)
				edges.InsertAt(nextEdge, 0);
			else
				edges.Insert(nextEdge);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendBounds(array<float> points, array<float> bounds)
	{
		float minX = points[0];
		float minZ = points[1];
		float maxX = minX;
		float maxZ = minZ;
		for (int i = 2; i < points.Count(); i += 2)
		{
			minX = Math.Min(minX, points[i]);
			maxX = Math.Max(maxX, points[i]);
			minZ = Math.Min(minZ, points[i + 1]);
			maxZ = Math.Max(maxZ, points[i + 1]);
		}

		bounds.Insert(minX);
		bounds.Insert(minZ);
		bounds.Insert(maxX);
		bounds.Insert(maxZ);
	}
}

//! HICOMM's zoomable strategic map, drawn on two CanvasWidgets in screen pixels (north up):
//! - base: sea, land, contours, adaptive grid, coastline glow and scanlines; rebuilt only when the view changes
//! - overlay: everything that moves (radar sweep, locations, squads, orders, selection); rebuilt every frame
class AROR_HiCommMap
{
	static const float MIN_ZOOM = 1;
	static const float MAX_ZOOM = 12;

	//! The base layer is rebuilt at most this often while panning/zooming (ms)
	protected static const int STATIC_REBUILD_MS = 40;

	//! Grid spacings (m), finest first; the finest one at least MIN_GRID_PIXELS apart is used
	protected static const float MIN_GRID_PIXELS = 55;

	//! Renderer limits: points per line command (2400 vertices max), smallest polygon drawn (twice the area, px²)
	protected static const int MAX_LINE_POINTS = 300;
	protected static const float MIN_POLYGON_AREA = 1;

	// Phosphor palette (packed sRGB). Dark tones are picked to stay distinct after conversion to the engine's
	// linear 8-bit canvas colours: deep sea darkest, land lighter with height.
	static const int SEA_DEEP = 0x020D0B;
	static const int SEA_MID = 0x06160F;
	static const int SEA_SHALLOW = 0x0D1C16;
	static const int COAST = 0x7CE6A8;
	static const int CONTOUR = 0x3C7E58;
	static const int GRID = 0x2A6446;

	//------------------------------------------------------------------------------------------------
	//! Land shade per elevation band (0 = lowest)
	protected static int LandShade(int band)
	{
		array<int> shades = {0x0D2216, 0x0D2618, 0x162A1C, 0x162E1C, 0x1C3222, 0x1C3726, 0x223C2A, 0x26422E};
		return shades[Math.ClampInt(band, 0, shades.Count() - 1)];
	}

	protected CanvasWidget m_wBase;
	protected CanvasWidget m_wOverlay;
	protected vector m_vMin;
	protected vector m_vMax;
	protected float m_fWidth;
	protected float m_fHeight;
	protected float m_fZoom = 1;
	protected vector m_vCentre;
	protected bool m_bHasView;

	protected bool m_bStaticDirty = true;
	protected int m_iLastStaticBuild;

	protected ref array<ref CanvasWidgetCommand> m_aBase = {};
	protected ref array<ref CanvasWidgetCommand> m_aOverlay = {};

	//------------------------------------------------------------------------------------------------
	void AROR_HiCommMap(CanvasWidget base, CanvasWidget overlay)
	{
		m_wBase = base;
		m_wOverlay = overlay;
	}

	//------------------------------------------------------------------------------------------------
	//! Sets the map area and the canvas size (pixels); keeps the current view if nothing changed
	void SetView(vector min, vector max, float width, float height)
	{
		if (!m_bHasView || min != m_vMin || max != m_vMax)
		{
			m_vMin = min;
			m_vMax = max;
			m_bHasView = true;
			Fit();
		}

		if (Math.AbsFloat(width - m_fWidth) > 0.5 || Math.AbsFloat(height - m_fHeight) > 0.5)
		{
			m_fWidth = width;
			m_fHeight = height;
			m_bStaticDirty = true;
		}
	}

	//------------------------------------------------------------------------------------------------
	float GetZoom()
	{
		return m_fZoom;
	}

	//------------------------------------------------------------------------------------------------
	float GetWidth()
	{
		return m_fWidth;
	}

	//------------------------------------------------------------------------------------------------
	float GetHeight()
	{
		return m_fHeight;
	}

	//------------------------------------------------------------------------------------------------
	//! At zoom 1 the whole map area fits the shorter side of the canvas
	float PixelsPerMetre()
	{
		float span = Math.Max(Math.Max(m_vMax[0] - m_vMin[0], m_vMax[2] - m_vMin[2]), 1);
		return m_fZoom * Math.Min(m_fWidth, m_fHeight) / span;
	}

	//------------------------------------------------------------------------------------------------
	vector WorldToScreen(vector position)
	{
		float scale = PixelsPerMetre();
		return Vector(m_fWidth * 0.5 + (position[0] - m_vCentre[0]) * scale, m_fHeight * 0.5 - (position[2] - m_vCentre[2]) * scale, 0);
	}

	//------------------------------------------------------------------------------------------------
	vector ScreenToWorld(float x, float y)
	{
		float scale = PixelsPerMetre();
		return Vector(m_vCentre[0] + (x - m_fWidth * 0.5) / scale, 0, m_vCentre[2] - (y - m_fHeight * 0.5) / scale);
	}

	//------------------------------------------------------------------------------------------------
	//! Zooms by factor, keeping the world point under (x, y) where it is
	void ZoomAt(float factor, float x, float y)
	{
		vector before = ScreenToWorld(x, y);
		m_fZoom = Math.Clamp(m_fZoom * factor, MIN_ZOOM, MAX_ZOOM);
		vector after = ScreenToWorld(x, y);
		m_vCentre = m_vCentre + before - after;
		ClampCentre();
		m_bStaticDirty = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Moves the view by screen pixels (drag direction: the map follows the cursor)
	void Pan(float dx, float dy)
	{
		float scale = PixelsPerMetre();
		m_vCentre[0] = m_vCentre[0] - dx / scale;
		m_vCentre[2] = m_vCentre[2] + dy / scale;
		ClampCentre();
		m_bStaticDirty = true;
	}

	//------------------------------------------------------------------------------------------------
	void CentreOn(vector position)
	{
		m_vCentre = Vector(position[0], 0, position[2]);
		ClampCentre();
		m_bStaticDirty = true;
	}

	//------------------------------------------------------------------------------------------------
	void Fit()
	{
		m_fZoom = MIN_ZOOM;
		m_vCentre = Vector((m_vMin[0] + m_vMax[0]) * 0.5, 0, (m_vMin[2] + m_vMax[2]) * 0.5);
		m_bStaticDirty = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void ClampCentre()
	{
		m_vCentre[0] = Math.Clamp(m_vCentre[0], m_vMin[0], m_vMax[0]);
		m_vCentre[2] = Math.Clamp(m_vCentre[2], m_vMin[2], m_vMax[2]);
	}

	//------------------------------------------------------------------------------------------------
	//! Grid line spacing (m) for the current zoom
	float GetGridSpacing()
	{
		array<float> spacings = {50, 100, 200, 500, 1000, 2000};
		float scale = PixelsPerMetre();
		foreach (float spacing : spacings)
		{
			if (spacing * scale >= MIN_GRID_PIXELS)
				return spacing;
		}

		return 2000;
	}

	//------------------------------------------------------------------------------------------------
	//! Visible world rectangle
	void GetVisibleBounds(out float minX, out float minZ, out float maxX, out float maxZ)
	{
		vector topLeft = ScreenToWorld(0, 0);
		vector bottomRight = ScreenToWorld(m_fWidth, m_fHeight);
		minX = topLeft[0];
		maxX = bottomRight[0];
		minZ = bottomRight[2];
		maxZ = topLeft[2];
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuilds the base layer if the view changed (rate-limited) or more terrain finished building
	void UpdateBase()
	{
		AROR_HiCommTerrain terrain = AROR_HiCommTerrain.Get(m_vMin, m_vMax);
		if (terrain.BuildStep())
			m_bStaticDirty = true;

		int now = System.GetTickCount();
		if (!m_bStaticDirty || now - m_iLastStaticBuild < STATIC_REBUILD_MS)
			return;

		m_bStaticDirty = false;
		m_iLastStaticBuild = now;
		BuildBase(terrain);
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildBase(AROR_HiCommTerrain terrain)
	{
		m_aBase.Clear();
		m_aBase.Insert(Polygon({0, 0, m_fWidth, 0, m_fWidth, m_fHeight, 0, m_fHeight}, Rgb(SEA_DEEP, 1)));

		float minX, minZ, maxX, maxZ;
		GetVisibleBounds(minX, minZ, maxX, maxZ);

		// Shaded water and land (elevation bands)
		array<ref Color> landShades = {};
		for (int shade = 0; shade < AROR_HiCommTerrain.LAND_BANDS; shade++)
		{
			landShades.Insert(Rgb(LandShade(shade), 1));
		}

		Color shallow = Rgb(SEA_SHALLOW, 1);
		Color middle = Rgb(SEA_MID, 1);
		foreach (int i, array<float> polygon : terrain.m_aLand)
		{
			if (Outside(terrain.m_aLandBounds, i, minX, minZ, maxX, maxZ))
				continue;

			int band = terrain.m_aLandBands[i];
			Color color = shallow;
			if (band == AROR_HiCommTerrain.SEA_MID)
				color = middle;
			else if (band > 0)
				color = landShades[Math.ClampInt(band - 1, 0, landShades.Count() - 1)];

			AddPolygon(m_aBase, ToScreen(polygon), color);
		}

		// Contours: faint when zoomed out, clearer as you zoom in
		float contourAlpha = Math.Clamp(0.25 + m_fZoom * 0.08, 0.25, 0.7);
		Color minor = Rgb(CONTOUR, contourAlpha * 0.6);
		Color major = Rgb(CONTOUR, contourAlpha);
		foreach (int i, array<float> line : terrain.m_aLines)
		{
			int kind = terrain.m_aLineKinds[i];
			if (kind == AROR_HiCommTerrain.LINE_COAST || Outside(terrain.m_aLineBounds, i, minX, minZ, maxX, maxZ))
				continue;

			if (kind == AROR_HiCommTerrain.LINE_CONTOUR_MAJOR)
				AddPolyline(m_aBase, ToScreen(line), major, 1.2, false);
			else
				AddPolyline(m_aBase, ToScreen(line), minor, 1, false);
		}

		AddGrid(minX, minZ, maxX, maxZ);

		// Coastline with a phosphor glow
		Color glow = Rgb(COAST, 0.16);
		Color coast = Rgb(COAST, 0.95);
		foreach (int i, array<float> line : terrain.m_aLines)
		{
			if (terrain.m_aLineKinds[i] != AROR_HiCommTerrain.LINE_COAST || Outside(terrain.m_aLineBounds, i, minX, minZ, maxX, maxZ))
				continue;

			array<float> screen = ToScreen(line);
			AddPolyline(m_aBase, screen, glow, 5, false);
			AddPolyline(m_aBase, screen, coast, 1.6, false);
		}

		// CRT scanlines
		Color scanline = Srgb(0, 0, 0, 0.22);
		for (float y = 0; y < m_fHeight; y += 3)
		{
			m_aBase.Insert(Line({0, y, m_fWidth, y}, scanline, 1, false));
		}

		m_wBase.SetDrawCommands(m_aBase);
	}

	//------------------------------------------------------------------------------------------------
	//! Adaptive grid: finer lines as you zoom in, 1 km lines always brighter
	protected void AddGrid(float minX, float minZ, float maxX, float maxZ)
	{
		float spacing = GetGridSpacing();
		Color minorColor = Rgb(GRID, 0.35);
		Color majorColor = Rgb(GRID, 0.8);

		for (float x = Math.Ceil(minX / spacing) * spacing; x <= maxX; x += spacing)
		{
			float screenX = WorldToScreen(Vector(x, 0, 0))[0];
			Color color = minorColor;
			if (IsMultiple(x, 1000))
				color = majorColor;

			m_aBase.Insert(Line({screenX, 0, screenX, m_fHeight}, color, 1, false));
		}

		for (float z = Math.Ceil(minZ / spacing) * spacing; z <= maxZ; z += spacing)
		{
			float screenY = WorldToScreen(Vector(0, 0, z))[1];
			Color color = minorColor;
			if (IsMultiple(z, 1000))
				color = majorColor;

			m_aBase.Insert(Line({0, screenY, m_fWidth, screenY}, color, 1, false));
		}
	}

	//------------------------------------------------------------------------------------------------
	static bool IsMultiple(float value, float step)
	{
		float steps = value / step;
		return Math.AbsFloat(steps - Math.Round(steps)) * step < 0.5;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool Outside(array<float> bounds, int index, float minX, float minZ, float maxX, float maxZ)
	{
		int i = index * 4;
		return bounds[i + 2] < minX || bounds[i] > maxX || bounds[i + 3] < minZ || bounds[i + 1] > maxZ;
	}

	//------------------------------------------------------------------------------------------------
	protected array<float> ToScreen(array<float> worldPoints)
	{
		float scale = PixelsPerMetre();
		float offsetX = m_fWidth * 0.5 - m_vCentre[0] * scale;
		float offsetY = m_fHeight * 0.5 + m_vCentre[2] * scale;
		array<float> screen = {};
		for (int i = 0; i < worldPoints.Count(); i += 2)
		{
			screen.Insert(offsetX + worldPoints[i] * scale);
			screen.Insert(offsetY - worldPoints[i + 1] * scale);
		}

		return screen;
	}

	//------------------------------------------------------------------------------------------------
	// Overlay (screen pixels)
	//------------------------------------------------------------------------------------------------
	void BeginOverlay()
	{
		m_aOverlay.Clear();
	}

	//------------------------------------------------------------------------------------------------
	void CommitOverlay()
	{
		m_wOverlay.SetDrawCommands(m_aOverlay);
	}

	//------------------------------------------------------------------------------------------------
	void DrawPolygon(array<float> vertices, Color color)
	{
		AddPolygon(m_aOverlay, vertices, color);
	}

	//------------------------------------------------------------------------------------------------
	void DrawPolyline(array<float> vertices, Color color, float width, bool enclose)
	{
		AddPolyline(m_aOverlay, vertices, color, width, enclose);
	}

	//------------------------------------------------------------------------------------------------
	void DrawLine(vector from, vector to, Color color, float width = 1.5)
	{
		AddPolyline(m_aOverlay, {from[0], from[1], to[0], to[1]}, color, width, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Line with a soft phosphor halo
	void DrawGlowLine(vector from, vector to, Color color, Color glow, float width = 2)
	{
		DrawLine(from, to, glow, width + 4);
		DrawLine(from, to, color, width);
	}

	//------------------------------------------------------------------------------------------------
	void DrawDashedLine(vector from, vector to, Color color, float width = 2, float dash = 8, float gap = 6)
	{
		float length = vector.Distance(from, to);
		if (length < 1)
			return;

		vector direction = (to - from) * (1.0 / length);
		for (float distance = 0; distance < length; distance += dash + gap)
		{
			float end = Math.Min(distance + dash, length);
			DrawLine(from + direction * distance, from + direction * end, color, width);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Filled arrowhead at "to", pointing away from "from"
	void DrawArrowHead(vector from, vector to, float size, Color color)
	{
		float length = vector.Distance(from, to);
		if (length < 1)
			return;

		vector direction = (to - from) * (1.0 / length);
		vector side = Vector(-direction[1], direction[0], 0);
		vector back = to - direction * size;
		DrawPolygon({to[0], to[1], back[0] + side[0] * size * 0.55, back[1] + side[1] * size * 0.55, back[0] - side[0] * size * 0.55, back[1] - side[1] * size * 0.55}, color);
	}

	//------------------------------------------------------------------------------------------------
	void DrawSquare(vector centre, float half, Color color, bool filled, float width = 1.5)
	{
		array<float> vertices = {centre[0] - half, centre[1] - half, centre[0] + half, centre[1] - half, centre[0] + half, centre[1] + half, centre[0] - half, centre[1] + half};
		if (filled)
			DrawPolygon(vertices, color);
		else
			DrawPolyline(vertices, color, width, true);
	}

	//------------------------------------------------------------------------------------------------
	void DrawDiamond(vector centre, float half, Color color, bool filled, float width = 1.5)
	{
		array<float> vertices = {centre[0], centre[1] - half, centre[0] + half, centre[1], centre[0], centre[1] + half, centre[0] - half, centre[1]};
		if (filled)
			DrawPolygon(vertices, color);
		else
			DrawPolyline(vertices, color, width, true);
	}

	//------------------------------------------------------------------------------------------------
	void DrawCircle(vector centre, float radius, Color color, bool filled, float width = 1.5, int segments = 20)
	{
		array<float> vertices = {};
		for (int i = 0; i < segments; i++)
		{
			float angle = Math.PI2 * i / segments;
			vertices.Insert(centre[0] + Math.Cos(angle) * radius);
			vertices.Insert(centre[1] + Math.Sin(angle) * radius);
		}

		if (filled)
			DrawPolygon(vertices, color);
		else
			DrawPolyline(vertices, color, width, true);
	}

	//------------------------------------------------------------------------------------------------
	//! NATO-style friendly infantry symbol (box with a cross) with echelon dots above
	void DrawUnit(vector centre, float half, Color color, Color fill, int echelonDots, bool selected)
	{
		float halfHeight = half * 0.68;
		array<float> box = {centre[0] - half, centre[1] - halfHeight, centre[0] + half, centre[1] - halfHeight, centre[0] + half, centre[1] + halfHeight, centre[0] - half, centre[1] + halfHeight};
		DrawPolygon(box, fill);
		DrawPolyline(box, color, 1.6, true);
		DrawLine(Vector(centre[0] - half, centre[1] - halfHeight, 0), Vector(centre[0] + half, centre[1] + halfHeight, 0), color, 1.2);
		DrawLine(Vector(centre[0] - half, centre[1] + halfHeight, 0), Vector(centre[0] + half, centre[1] - halfHeight, 0), color, 1.2);

		for (int dot = 0; dot < echelonDots; dot++)
		{
			float offset = (dot - (echelonDots - 1) * 0.5) * 5;
			DrawSquare(Vector(centre[0] + offset, centre[1] - halfHeight - 5, 0), 1.3, color, true);
		}

		if (!selected)
			return;

		// Selection brackets
		float gap = half + 5;
		float arm = 5;
		float bracketHeight = halfHeight + 6;
		for (int side = -1; side <= 1; side += 2)
		{
			float x = centre[0] + gap * side;
			DrawLine(Vector(x, centre[1] - bracketHeight, 0), Vector(x, centre[1] + bracketHeight, 0), color, 1.6);
			DrawLine(Vector(x, centre[1] - bracketHeight, 0), Vector(x - arm * side, centre[1] - bracketHeight, 0), color, 1.6);
			DrawLine(Vector(x, centre[1] + bracketHeight, 0), Vector(x - arm * side, centre[1] + bracketHeight, 0), color, 1.6);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Target reticle: ring with ticks
	void DrawReticle(vector centre, float radius, Color color)
	{
		DrawCircle(centre, radius, color, false, 2);
		float inner = radius * 0.5;
		float outer = radius * 1.6;
		DrawLine(Vector(centre[0] - outer, centre[1], 0), Vector(centre[0] - inner, centre[1], 0), color, 2);
		DrawLine(Vector(centre[0] + inner, centre[1], 0), Vector(centre[0] + outer, centre[1], 0), color, 2);
		DrawLine(Vector(centre[0], centre[1] - outer, 0), Vector(centre[0], centre[1] - inner, 0), color, 2);
		DrawLine(Vector(centre[0], centre[1] + inner, 0), Vector(centre[0], centre[1] + outer, 0), color, 2);
	}

	//------------------------------------------------------------------------------------------------
	//! Radar-scope corner brackets around the whole canvas
	void DrawFrameCorners(Color color)
	{
		float arm = 26;
		float inset = 6;
		float right = m_fWidth - inset;
		float bottom = m_fHeight - inset;
		DrawPolyline({inset, inset + arm, inset, inset, inset + arm, inset}, color, 2, false);
		DrawPolyline({right - arm, inset, right, inset, right, inset + arm}, color, 2, false);
		DrawPolyline({inset, bottom - arm, inset, bottom, inset + arm, bottom}, color, 2, false);
		DrawPolyline({right - arm, bottom, right, bottom, right, bottom - arm}, color, 2, false);
	}

	//------------------------------------------------------------------------------------------------
	// Helpers
	//------------------------------------------------------------------------------------------------
	//! UI colours are applied in linear space, so designed (sRGB) values are converted first; without this, dark
	//! tones come out washed out. Channels 0-255, alpha 0-1.
	static Color Srgb(int red, int green, int blue, float alpha)
	{
		return new Color(ToLinear(red), ToLinear(green), ToLinear(blue), Math.Clamp(alpha, 0, 1));
	}

	//------------------------------------------------------------------------------------------------
	//! Packed 0xRRGGBB (sRGB) with an alpha of 0-1
	static Color Rgb(int packed, float alpha)
	{
		return Srgb((packed >> 16) & 0xFF, (packed >> 8) & 0xFF, packed & 0xFF, alpha);
	}

	//------------------------------------------------------------------------------------------------
	protected static float ToLinear(int channel)
	{
		float value = Math.Clamp(channel / 255.0, 0, 1);
		if (value <= 0.04045)
			return value / 12.92;

		return Math.Pow((value + 0.055) / 1.055, 2.4);
	}

	//------------------------------------------------------------------------------------------------
	// Clipping: canvases do not clip their drawing, so everything is cut to the canvas rectangle here
	//------------------------------------------------------------------------------------------------
	//! Convex polygon (Sutherland-Hodgman against the four edges)
	protected void AddPolygon(array<ref CanvasWidgetCommand> target, array<float> vertices, Color color)
	{
		int count = vertices.Count();
		if (count < 6)
			return;

		float minX = vertices[0];
		float maxX = minX;
		float minY = vertices[1];
		float maxY = minY;
		for (int i = 2; i < count; i += 2)
		{
			minX = Math.Min(minX, vertices[i]);
			maxX = Math.Max(maxX, vertices[i]);
			minY = Math.Min(minY, vertices[i + 1]);
			maxY = Math.Max(maxY, vertices[i + 1]);
		}

		if (maxX < 0 || maxY < 0 || minX > m_fWidth || minY > m_fHeight)
			return;

		array<float> clipped = vertices;
		if (minX < 0 || minY < 0 || maxX > m_fWidth || maxY > m_fHeight)
		{
			clipped = ClipEdge(vertices, 0, 0, false);
			clipped = ClipEdge(clipped, 0, m_fWidth, true);
			clipped = ClipEdge(clipped, 1, 0, false);
			clipped = ClipEdge(clipped, 1, m_fHeight, true);
		}

		// Skip slivers (e.g. a coastal cell barely touching land): invisible, and they fail triangulation
		if (clipped.Count() >= 6 && Math.AbsFloat(DoubleArea(clipped)) > MIN_POLYGON_AREA)
			target.Insert(Polygon(clipped, color));
	}

	//------------------------------------------------------------------------------------------------
	//! Keeps the part of the polygon on the inside of one edge: axis 0 = x, 1 = y; keepBelow = inside is < limit
	protected static array<float> ClipEdge(array<float> vertices, int axis, float limit, bool keepBelow)
	{
		array<float> result = {};
		int count = vertices.Count() / 2;
		for (int i = 0; i < count; i++)
		{
			int next = Math.Mod(i + 1, count);
			float ax = vertices[i * 2];
			float ay = vertices[i * 2 + 1];
			float bx = vertices[next * 2];
			float by = vertices[next * 2 + 1];
			float a = ay;
			float b = by;
			if (axis == 0)
			{
				a = ax;
				b = bx;
			}

			bool aInside = (keepBelow && a <= limit) || (!keepBelow && a >= limit);
			bool bInside = (keepBelow && b <= limit) || (!keepBelow && b >= limit);
			if (aInside)
			{
				result.Insert(ax);
				result.Insert(ay);
			}

			if (aInside != bInside && Math.AbsFloat(b - a) > 0.0001)
			{
				float t = (limit - a) / (b - a);
				result.Insert(ax + (bx - ax) * t);
				result.Insert(ay + (by - ay) * t);
			}
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Polyline cut into the visible runs (Liang-Barsky per segment)
	protected void AddPolyline(array<ref CanvasWidgetCommand> target, array<float> vertices, Color color, float width, bool enclose)
	{
		// Wide lines get long mitre spikes where they double back (e.g. the coastline round a small inlet):
		// break them at sharp turns so no join is ever that sharp
		if (width >= 2 && !enclose)
		{
			array<ref array<float>> pieces = {};
			SplitAtSharpTurns(vertices, pieces);
			if (pieces.Count() > 1)
			{
				foreach (array<float> piece : pieces)
				{
					ClipPolyline(target, piece, color, width, false);
				}

				return;
			}
		}

		ClipPolyline(target, vertices, color, width, enclose);
	}

	//------------------------------------------------------------------------------------------------
	//! Splits a polyline wherever its direction turns by more than about 110 degrees (pieces overlap by one point)
	protected static void SplitAtSharpTurns(array<float> vertices, array<ref array<float>> pieces)
	{
		int count = vertices.Count() / 2;
		array<float> piece = {vertices[0], vertices[1]};
		for (int i = 1; i < count; i++)
		{
			piece.Insert(vertices[i * 2]);
			piece.Insert(vertices[i * 2 + 1]);
			if (i == count - 1)
				break;

			float inX = vertices[i * 2] - vertices[i * 2 - 2];
			float inY = vertices[i * 2 + 1] - vertices[i * 2 - 1];
			float outX = vertices[i * 2 + 2] - vertices[i * 2];
			float outY = vertices[i * 2 + 3] - vertices[i * 2 + 1];
			float lengths = Math.Sqrt((inX * inX + inY * inY) * (outX * outX + outY * outY));
			if (lengths > 0.0001 && (inX * outX + inY * outY) / lengths < -0.34)
			{
				pieces.Insert(piece);
				piece = {vertices[i * 2], vertices[i * 2 + 1]};
			}
		}

		pieces.Insert(piece);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClipPolyline(array<ref CanvasWidgetCommand> target, array<float> vertices, Color color, float width, bool enclose)
	{
		int count = vertices.Count() / 2;
		if (count < 2)
			return;

		int segments = count - 1;
		if (enclose)
			segments = count;

		array<float> run = {};
		for (int i = 0; i < segments; i++)
		{
			int next = Math.Mod(i + 1, count);
			float x0 = vertices[i * 2];
			float y0 = vertices[i * 2 + 1];
			float x1 = vertices[next * 2];
			float y1 = vertices[next * 2 + 1];

			float start, end;
			if (!ClipSegment(x0, y0, x1, y1, start, end))
			{
				FlushRun(target, run, color, width);
				continue;
			}

			float sx = x0 + (x1 - x0) * start;
			float sy = y0 + (y1 - y0) * start;
			float ex = x0 + (x1 - x0) * end;
			float ey = y0 + (y1 - y0) * end;

			// Continue the run only if this segment starts where the last visible one ended
			if (!run.IsEmpty() && start > 0)
				FlushRun(target, run, color, width);

			if (run.IsEmpty())
			{
				run.Insert(sx);
				run.Insert(sy);
			}

			run.Insert(ex);
			run.Insert(ey);

			if (end < 1)
				FlushRun(target, run, color, width);
		}

		FlushRun(target, run, color, width);
	}

	//------------------------------------------------------------------------------------------------
	//! The renderer limits one line to 2400 vertices (about 400 points), so long runs go out in overlapping chunks
	protected static void FlushRun(array<ref CanvasWidgetCommand> target, array<float> run, Color color, float width)
	{
		int count = run.Count() / 2;
		int start = 0;
		while (count - start >= 2)
		{
			int end = Math.Min(start + MAX_LINE_POINTS, count);
			array<float> points = {};
			for (int i = start; i < end; i++)
			{
				points.Insert(run[i * 2]);
				points.Insert(run[i * 2 + 1]);
			}

			target.Insert(Line(points, color, width, false));
			if (end >= count)
				break;

			start = end - 1;
		}

		run.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! Twice the signed area; near zero means a sliver the renderer cannot triangulate
	protected static float DoubleArea(array<float> vertices)
	{
		float area;
		int count = vertices.Count() / 2;
		for (int i = 0; i < count; i++)
		{
			int next = Math.Mod(i + 1, count);
			area += vertices[i * 2] * vertices[next * 2 + 1] - vertices[next * 2] * vertices[i * 2 + 1];
		}

		return area;
	}

	//------------------------------------------------------------------------------------------------
	//! Visible parameter range [start, end] of the segment inside the canvas; false if none
	protected bool ClipSegment(float x0, float y0, float x1, float y1, out float start, out float end)
	{
		start = 0;
		end = 1;
		float dx = x1 - x0;
		float dy = y1 - y0;
		array<float> p = {-dx, dx, -dy, dy};
		array<float> q = {x0, m_fWidth - x0, y0, m_fHeight - y0};
		for (int i = 0; i < 4; i++)
		{
			if (Math.AbsFloat(p[i]) < 0.00001)
			{
				if (q[i] < 0)
					return false;

				continue;
			}

			float t = q[i] / p[i];
			if (p[i] < 0)
				start = Math.Max(start, t);
			else
				end = Math.Min(end, t);
		}

		return start <= end;
	}

	//------------------------------------------------------------------------------------------------
	protected static PolygonDrawCommand Polygon(array<float> vertices, Color color)
	{
		PolygonDrawCommand command = new PolygonDrawCommand();
		command.m_iColor = color.PackToInt();
		command.m_Vertices = vertices;
		return command;
	}

	//------------------------------------------------------------------------------------------------
	protected static LineDrawCommand Line(array<float> vertices, Color color, float width, bool enclose)
	{
		LineDrawCommand command = new LineDrawCommand();
		command.m_iColor = color.PackToInt();
		command.m_fWidth = width;
		command.m_Vertices = vertices;
		command.m_bShouldEnclose = enclose;
		return command;
	}
}
