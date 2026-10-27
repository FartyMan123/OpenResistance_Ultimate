//! Passes clicks on a pooled app's buttons to the app
class AROR_PhonePoolClickHandler : ScriptedWidgetEventHandler
{
	protected AROR_PhonePooledApp m_App;

	//------------------------------------------------------------------------------------------------
	void AROR_PhonePoolClickHandler(AROR_PhonePooledApp app)
	{
		m_App = app;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_App)
			m_App.OnPoolClick(w);

		return true;
	}
}

//! Base for landscape phone apps drawn immediate-mode from fixed widget pools, as HICOMM does. The layout
//! (AROR_PhoneMenu.layout) declares, under the app's root frame, <prefix>Background and pools named <prefix>Panel<i>
//! (ImageWidget), <prefix>Text<i> (RichTextWidget) and <prefix>BtnBg<i> / <prefix>BtnLabel<i> / <prefix>Btn<i>
//! (button background, label and click area). Each frame the app draws what it needs between BeginFrame and
//! EndFrame, in app units (the 1700 x 900 landscape screen) times the phone's scale; unused widgets are hidden and
//! unchanged properties are not set again. Buttons carry an action string for that frame (see DoAction).
class AROR_PhonePooledApp
{
	protected static const float WIDTH = 1700;
	protected static const float HEIGHT = 900;

	//! Rough width of a character as a fraction of the font size, used to centre, right-align and wrap text
	protected static const float CHAR_WIDTH_FACTOR = 0.55;
	protected static const float WRAP_CHAR_FACTOR = 0.58;

	protected string m_sPrefix;
	protected Widget m_wRoot;
	protected ImageWidget m_wBackground;
	protected ref AROR_PhonePoolClickHandler m_ClickHandler;

	protected ref array<ImageWidget> m_aPanels = {};
	protected ref array<RichTextWidget> m_aTexts = {};
	protected ref array<string> m_aTextCache = {};
	protected ref array<ImageWidget> m_aButtonBackgrounds = {};
	protected ref array<RichTextWidget> m_aButtonLabels = {};
	protected ref array<string> m_aButtonLabelCache = {};
	protected ref array<Widget> m_aButtons = {};
	protected ref array<string> m_aButtonActions = {};

	protected ref array<ref AROR_HiCommWidgetCache> m_aPanelCaches = {};
	protected ref array<ref AROR_HiCommWidgetCache> m_aTextCaches = {};
	protected ref array<ref AROR_HiCommWidgetCache> m_aButtonBgCaches = {};
	protected ref array<ref AROR_HiCommWidgetCache> m_aButtonLabelCaches = {};
	protected ref array<ref AROR_HiCommWidgetCache> m_aButtonCaches = {};

	protected int m_iPanelCount;
	protected int m_iTextCount;
	protected int m_iButtonCount;

	protected float m_fScale = 1;
	protected float m_fInsetX;
	protected float m_fInsetY;

	//------------------------------------------------------------------------------------------------
	//! Finds the root, background and pools; false if the layout lacks any of them
	protected bool InitPools(Widget phoneRoot, string prefix, int panels, int texts, int buttons)
	{
		m_sPrefix = prefix;
		m_wRoot = phoneRoot.FindAnyWidget(prefix + "App");
		m_wBackground = ImageWidget.Cast(phoneRoot.FindAnyWidget(prefix + "Background"));
		if (!m_wRoot || !m_wBackground)
			return false;

		m_ClickHandler = new AROR_PhonePoolClickHandler(this);

		for (int i = 0; i < panels; i++)
		{
			ImageWidget panel = ImageWidget.Cast(phoneRoot.FindAnyWidget(prefix + "Panel" + i));
			if (!panel)
				return false;

			m_aPanels.Insert(panel);
			m_aPanelCaches.Insert(new AROR_HiCommWidgetCache());
		}

		for (int t = 0; t < texts; t++)
		{
			RichTextWidget text = RichTextWidget.Cast(phoneRoot.FindAnyWidget(prefix + "Text" + t));
			if (!text)
				return false;

			m_aTexts.Insert(text);
			m_aTextCache.Insert(string.Empty);
			m_aTextCaches.Insert(new AROR_HiCommWidgetCache());
		}

		for (int b = 0; b < buttons; b++)
		{
			ImageWidget background = ImageWidget.Cast(phoneRoot.FindAnyWidget(prefix + "BtnBg" + b));
			RichTextWidget label = RichTextWidget.Cast(phoneRoot.FindAnyWidget(prefix + "BtnLabel" + b));
			Widget button = phoneRoot.FindAnyWidget(prefix + "Btn" + b);
			if (!background || !label || !button)
				return false;

			button.AddHandler(m_ClickHandler);
			button.SetColor(Color.FromRGBA(0, 0, 0, 0));
			m_aButtonBackgrounds.Insert(background);
			m_aButtonLabels.Insert(label);
			m_aButtonLabelCache.Insert(string.Empty);
			m_aButtons.Insert(button);
			m_aButtonActions.Insert(string.Empty);
			m_aButtonBgCaches.Insert(new AROR_HiCommWidgetCache());
			m_aButtonLabelCaches.Insert(new AROR_HiCommWidgetCache());
			m_aButtonCaches.Insert(new AROR_HiCommWidgetCache());
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	void SetVisible(bool visible, float opacity)
	{
		m_wRoot.SetVisible(visible);
		m_wRoot.SetOpacity(opacity);
	}

	//------------------------------------------------------------------------------------------------
	//! Places the app over the landscape screen and starts a frame; content is inset to contentScale of the screen
	protected void BeginFrame(float left, float top, float scale, float contentScale)
	{
		m_fScale = scale * contentScale;
		m_fInsetX = WIDTH * (scale - m_fScale) * 0.5;
		m_fInsetY = HEIGHT * (scale - m_fScale) * 0.5;
		PlaceRaw(m_wRoot, left, top, WIDTH * scale, HEIGHT * scale);
		PlaceRaw(m_wBackground, 0, 0, WIDTH * scale, HEIGHT * scale);

		m_iPanelCount = 0;
		m_iTextCount = 0;
		m_iButtonCount = 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Hides every pooled widget this frame did not use
	protected void EndFrame()
	{
		for (int i = m_iPanelCount; i < m_aPanels.Count(); i++)
		{
			CachedVisible(m_aPanels[i], m_aPanelCaches[i], false);
		}

		for (int t = m_iTextCount; t < m_aTexts.Count(); t++)
		{
			CachedVisible(m_aTexts[t], m_aTextCaches[t], false);
		}

		for (int b = m_iButtonCount; b < m_aButtons.Count(); b++)
		{
			CachedVisible(m_aButtonBackgrounds[b], m_aButtonBgCaches[b], false);
			CachedVisible(m_aButtonLabels[b], m_aButtonLabelCaches[b], false);
			CachedVisible(m_aButtons[b], m_aButtonCaches[b], false);
			m_aButtonActions[b] = string.Empty;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! A pooled button was clicked
	void OnPoolClick(Widget w)
	{
		if (!w || !m_wRoot.IsVisible())
			return;

		string name = w.GetName();
		string buttonPrefix = m_sPrefix + "Btn";
		if (name.IndexOf(buttonPrefix) != 0)
			return;

		int index = name.Substring(buttonPrefix.Length(), name.Length() - buttonPrefix.Length()).ToInt();
		if (!m_aButtonActions.IsIndexValid(index) || index >= m_iButtonCount)
			return;

		string action = m_aButtonActions[index];
		if (!action.IsEmpty())
			DoAction(action);
	}

	//------------------------------------------------------------------------------------------------
	//! Handles a button's action string
	protected void DoAction(string action)
	{
	}

	//------------------------------------------------------------------------------------------------
	// Drawing (app units)
	//------------------------------------------------------------------------------------------------
	protected void Panel(float x, float y, float width, float height, Color color)
	{
		if (m_iPanelCount >= m_aPanels.Count())
			return;

		ImageWidget panel = m_aPanels[m_iPanelCount];
		AROR_HiCommWidgetCache cache = m_aPanelCaches[m_iPanelCount];
		CachedColor(panel, cache, color);
		CachedPlace(panel, cache, x * m_fScale, y * m_fScale, width * m_fScale, height * m_fScale);
		CachedVisible(panel, cache, true);
		m_iPanelCount++;
	}

	//------------------------------------------------------------------------------------------------
	//! Rectangle outline of the given thickness
	protected void Frame(float x, float y, float width, float height, float thickness, Color color)
	{
		Panel(x, y, width, thickness, color);
		Panel(x, y + height - thickness, width, thickness, color);
		Panel(x, y + thickness, thickness, height - thickness * 2, color);
		Panel(x + width - thickness, y + thickness, thickness, height - thickness * 2, color);
	}

	//------------------------------------------------------------------------------------------------
	//! Text wrapped to width (by estimated character width); height is the box it may use
	protected void Text(float x, float y, float width, float height, float size, string content, Color color, bool bold = false)
	{
		if (m_iTextCount >= m_aTexts.Count())
			return;

		int index = m_iTextCount;
		RichTextWidget text = m_aTexts[index];
		AROR_HiCommWidgetCache cache = m_aTextCaches[index];

		if (cache.m_sRaw != content || cache.m_fWrapWidth != width || cache.m_fWrapSize != size)
		{
			cache.m_sRaw = content;
			cache.m_fWrapWidth = width;
			cache.m_fWrapSize = size;
			string wrapped = Wrap(content, width, size);
			if (m_aTextCache[index] != wrapped)
			{
				text.SetText(wrapped);
				m_aTextCache[index] = wrapped;
			}
		}

		CachedColor(text, cache, color);
		CachedBold(text, cache, bold);
		CachedFontSize(text, cache, size * m_fScale);
		CachedPlace(text, cache, x * m_fScale, y * m_fScale, width * m_fScale, height * m_fScale);
		CachedVisible(text, cache, true);
		m_iTextCount++;
	}

	//------------------------------------------------------------------------------------------------
	//! Single line ending at right
	protected void TextRight(float right, float y, float size, string content, Color color, bool bold = false)
	{
		float width = TextWidth(content, size);
		Text(right - width, y, width + size, size * 1.5, size, content, color, bold);
	}

	//------------------------------------------------------------------------------------------------
	//! Single line centred on x
	protected void TextCentre(float x, float y, float size, string content, Color color, bool bold = false)
	{
		float width = TextWidth(content, size);
		Text(x - width * 0.5, y, width + size, size * 1.5, size, content, color, bold);
	}

	//------------------------------------------------------------------------------------------------
	//! Estimated width of a single line (app units)
	protected static float TextWidth(string content, float size)
	{
		return content.Length() * size * CHAR_WIDTH_FACTOR;
	}

	//------------------------------------------------------------------------------------------------
	//! Lines a text takes when wrapped to width
	protected static int LineCount(string content, float width, float size)
	{
		string wrapped = Wrap(content, width, size);
		array<string> lines = {};
		wrapped.Split("\n", lines, false);
		return Math.Max(lines.Count(), 1);
	}

	//------------------------------------------------------------------------------------------------
	//! A filled button with a label (centred, or left-aligned with padding); empty action = not clickable
	protected void Button(float x, float y, float width, float height, string label, string action, Color fill, Color labelColor, float size, bool centred = true, bool bold = true)
	{
		if (m_iButtonCount >= m_aButtons.Count())
			return;

		int index = m_iButtonCount;
		AROR_HiCommWidgetCache labelCache = m_aButtonLabelCaches[index];
		int centredValue = 0;
		if (centred)
			centredValue = 1;

		if (labelCache.m_sRaw != label || labelCache.m_fWrapWidth != width || labelCache.m_fWrapSize != size || labelCache.m_iCentred != centredValue)
		{
			labelCache.m_sRaw = label;
			labelCache.m_fWrapWidth = width;
			labelCache.m_fWrapSize = size;
			labelCache.m_iCentred = centredValue;
			labelCache.m_iLongest = label.Length();
			labelCache.m_iLines = 1;
			if (m_aButtonLabelCache[index] != label)
			{
				m_aButtonLabels[index].SetText(label);
				m_aButtonLabelCache[index] = label;
			}
		}

		ImageWidget background = m_aButtonBackgrounds[index];
		AROR_HiCommWidgetCache backgroundCache = m_aButtonBgCaches[index];
		CachedColor(background, backgroundCache, fill);
		CachedPlace(background, backgroundCache, x * m_fScale, y * m_fScale, width * m_fScale, height * m_fScale);
		CachedVisible(background, backgroundCache, true);

		RichTextWidget text = m_aButtonLabels[index];
		CachedColor(text, labelCache, labelColor);
		CachedBold(text, labelCache, bold);
		CachedFontSize(text, labelCache, size * m_fScale);
		float labelX = x + size * 0.8;
		if (centred)
			labelX = x + Math.Max((width - labelCache.m_iLongest * size * CHAR_WIDTH_FACTOR) * 0.5, 4);

		float labelHeight = size * 1.3;
		CachedPlace(text, labelCache, labelX * m_fScale, (y + (height - labelHeight) * 0.5) * m_fScale, (x + width - labelX) * m_fScale, (labelHeight + size * 0.2) * m_fScale);
		CachedVisible(text, labelCache, true);

		Widget button = m_aButtons[index];
		AROR_HiCommWidgetCache buttonCache = m_aButtonCaches[index];
		CachedPlace(button, buttonCache, x * m_fScale, y * m_fScale, width * m_fScale, height * m_fScale);
		CachedVisible(button, buttonCache, !action.IsEmpty());
		m_aButtonActions[index] = action;
		m_iButtonCount++;
	}

	//------------------------------------------------------------------------------------------------
	//! An invisible click area (e.g. over a list row drawn from panels and texts)
	protected void HitArea(float x, float y, float width, float height, string action)
	{
		Button(x, y, width, height, string.Empty, action, Color.FromRGBA(0, 0, 0, 0), Color.FromRGBA(0, 0, 0, 0), 10);
	}

	//------------------------------------------------------------------------------------------------
	// Cached widget setters: engine calls are only made when a value actually changed
	//------------------------------------------------------------------------------------------------
	protected void CachedPlace(Widget widget, AROR_HiCommWidgetCache cache, float x, float y, float width, float height)
	{
		x += m_fInsetX;
		y += m_fInsetY;
		if (Math.AbsFloat(x - cache.m_fX) < 0.05 && Math.AbsFloat(y - cache.m_fY) < 0.05 && Math.AbsFloat(width - cache.m_fWidth) < 0.05 && Math.AbsFloat(height - cache.m_fHeight) < 0.05)
			return;

		cache.m_fX = x;
		cache.m_fY = y;
		cache.m_fWidth = width;
		cache.m_fHeight = height;
		PlaceRaw(widget, x, y, width, height);
	}

	//------------------------------------------------------------------------------------------------
	protected static void CachedColor(Widget widget, AROR_HiCommWidgetCache cache, Color color)
	{
		int packed = color.PackToInt();
		if (cache.m_bColorSet && cache.m_iColor == packed)
			return;

		cache.m_bColorSet = true;
		cache.m_iColor = packed;
		widget.SetColor(color);
	}

	//------------------------------------------------------------------------------------------------
	protected static void CachedFontSize(TextWidget text, AROR_HiCommWidgetCache cache, float size)
	{
		int fontSize = Math.Max(1, Math.Round(size));
		if (cache.m_iFontSize == fontSize)
			return;

		cache.m_iFontSize = fontSize;
		text.SetExactFontSize(fontSize);
	}

	//------------------------------------------------------------------------------------------------
	protected static void CachedBold(TextWidget text, AROR_HiCommWidgetCache cache, bool bold)
	{
		int value = 0;
		if (bold)
			value = 1;

		if (cache.m_iBold == value)
			return;

		cache.m_iBold = value;
		text.SetBold(bold);
	}

	//------------------------------------------------------------------------------------------------
	protected static void CachedVisible(Widget widget, AROR_HiCommWidgetCache cache, bool visible)
	{
		int value = 0;
		if (visible)
			value = 1;

		if (cache.m_iVisible == value)
			return;

		cache.m_iVisible = value;
		widget.SetVisible(visible);
	}

	//------------------------------------------------------------------------------------------------
	//! Places a non-pooled widget in app units (inside the inset content area)
	protected void Place(Widget widget, float x, float y, float width, float height)
	{
		PlaceRaw(widget, x + m_fInsetX, y + m_fInsetY, width, height);
	}

	//------------------------------------------------------------------------------------------------
	protected static void PlaceRaw(Widget widget, float x, float y, float width, float height)
	{
		if (!widget)
			return;

		FrameSlot.SetPos(widget, x, y);
		FrameSlot.SetSize(widget, width, height);
	}

	//------------------------------------------------------------------------------------------------
	// Text helpers
	//------------------------------------------------------------------------------------------------
	//! Inserts line breaks so each line fits width at size (estimated); existing line breaks are kept
	protected static string Wrap(string content, float width, float size)
	{
		int maxChars = Math.Max(Math.Floor(width / (size * WRAP_CHAR_FACTOR)), 4);
		if (content.Length() <= maxChars && content.IndexOf("\n") < 0)
			return content;

		array<string> paragraphs = {};
		content.Split("\n", paragraphs, false);
		string result;
		foreach (int p, string paragraph : paragraphs)
		{
			if (p > 0)
				result += "\n";

			array<string> words = {};
			paragraph.Split(" ", words, false);
			string line;
			foreach (int w, string word : words)
			{
				if (w == 0)
					line = word;
				else if (line.Length() + 1 + word.Length() <= maxChars)
					line += " " + word;
				else
				{
					result += line + "\n";
					line = word;
				}
			}

			result += line;
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! string.ToUpper changes the string in place; this returns an upper-case copy
	protected static string Upper(string text)
	{
		text.ToUpper();
		return text;
	}

	//------------------------------------------------------------------------------------------------
	protected static int GetLocalPlayerId()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return 0;

		return controller.GetPlayerId();
	}

	//------------------------------------------------------------------------------------------------
	protected static string GetPlayerName(int playerId)
	{
		string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
		if (name.IsEmpty())
			return "UNKNOWN";

		return name;
	}

	//------------------------------------------------------------------------------------------------
	//! In-game time of day, HH:MM
	protected static string GetClockText()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world || !world.GetTimeAndWeatherManager())
			return "--:--";

		int hours, minutes, seconds;
		world.GetTimeAndWeatherManager().GetHoursMinutesSeconds(hours, minutes, seconds);
		return hours.ToString(2) + ":" + minutes.ToString(2);
	}
}
