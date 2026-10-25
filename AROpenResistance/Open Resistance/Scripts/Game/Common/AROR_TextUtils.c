//! Shared text helpers for UI
class AROR_TextUtils
{
	//------------------------------------------------------------------------------------------------
	//! Enum value name to display text: "MILITARY_BASE" -> "Military Base"
	static string FormatEnumName(string enumName)
	{
		array<string> words = {};
		enumName.Split("_", words, true);

		string result;
		foreach (int i, string word : words)
		{
			if (i > 0)
				result += " ";

			// ToLower/ToUpper modify the string in place
			word.ToLower();
			string first = word.Substring(0, 1);
			first.ToUpper();
			result += first + word.Substring(1, word.Length() - 1);
		}

		return result;
	}
}
