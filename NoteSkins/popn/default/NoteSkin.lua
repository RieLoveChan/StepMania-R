local ret = ... or {};

ret.RedirTable =
{
	["Left White"] = "Keyw",
	["Left Yellow"] = "Keyy",
	["Left Green"] = "Keyg",
	["Left Blue"] = "Keyb",
	Red = "Keyr",
	["Right White"] = "RKeyw",
	["Right Yellow"] = "RKeyy",
	["Right Green"] = "RKeyg",
	["Right Blue"] = "RKeyb",
};

local OldRedir = ret.Redir;
ret.Redir = function(sButton, sElement)
	sButton, sElement = OldRedir(sButton, sElement);
	-- Instead of separate hold heads, use the tap note graphics.
	if sElement == "Hold Head Inactive" or
	   sElement == "Hold Head Active" or
	   sElement == "Roll Head Inactive" or
	   sElement == "Roll Head Active"
	then
		sElement = "Tap Note";
	end
	sButton = ret.RedirTable[sButton];
	return sButton, sElement;
end

ret.PartsToRotate =
{
	["Receptor"] = true,
};

ret.Rotate =
{
	["Left White"] = 0,
	["Left Yellow"] = 0,
	["Left Green"] = 0,
	["Left Blue"] = 0,
	Red = 0,
	["Right White"] = 0,
	["Right Yellow"] = 0,
	["Right Green"] = 0,
	["Right Blue"] = 0,
};

return ret;