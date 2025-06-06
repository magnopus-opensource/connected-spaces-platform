"""
Implementation of a simple string tokenizer with lookahead and skipping capabilities.
"""

class WordReader:
    """
    Implements the string tokenizer.

    >>> w = WordReader("Hello, world!")
    >>> w.next_word()
    'Hello'
    >>> w.next_word()
    ','
    >>> w.next_word()
    'world!'
    """

    text: str
    index: int
    current_line: int = 1

    WHITESPACE_CHARS = set([" ", "\t", "\r", "\n"])
    DEFAULT_DELIMITERS = set(
        [
            " ",
            "\t",
            "\n",
            ":",
            ";",
            "{",
            "}",
            ",",
            "<",
            ">",
            "(",
            ")",
            "=",
            "#",
            "~",
            "*",
            "&",
            "[",
            "]",
        ]
    )

    def __init__(self, text: str):
        self.text = text
        self.index = 0

    def skip_whitespace(self) -> None:
        """
        Moves the reading index forward, skipping all whitespace characters until a non-whitespace character or end-of-file is reached.
        If a newline character is encountered, increments the current line counter.
        Examples:

        >>> w = WordReader("  Meow")
        >>> w.skip_whitespace()
        >>> w.peek_char()
        'M'
        """
        while (
            self.index != len(self.text)
            and self.text[self.index] in self.WHITESPACE_CHARS
        ):
            if self.text[self.index] == "\n":
                self.current_line += 1

            self.index += 1

    def next_word(self, delimiters: set[str] = None, return_empty: bool = False) -> str:
        """
        Returns the next word from the text, using the specified delimiters.
        Args:
            delimiters (set[str], optional): A set of delimiter characters to split words.
                If None, uses the default delimiters.
            return_empty (bool, optional): If True, allows returning empty words.
                If False, skips empty words. Defaults to False.
        Returns:
            str or None: The next word in the text, or None if the end of the text is reached.

        Example:

        >>> w = WordReader("Hello, world!")
        >>> w.next_word()
        'Hello'
        >>> w.next_word()
        ','
        >>> w.next_word()
        'world!'
        >>> w.next_word()
        None
        """
        if delimiters is None:
            delimiters = self.DEFAULT_DELIMITERS

        if self.index == len(self.text):
            return None

        start_index = self.index
        # Always consume the first character
        end_index = self.index + 1

        while True:
            next_char = self.peek_char()

            if next_char == "\n":
                self.current_line += 1

            if next_char in delimiters and next_char not in self.WHITESPACE_CHARS:
                self.index = end_index
                self.skip_whitespace()

                return self.text[start_index]

            while (
                end_index != len(self.text) and self.text[end_index] not in delimiters
            ):
                end_index += 1

            word = self.text[start_index:end_index].strip()

            if return_empty or word != "":
                self.index = end_index
                self.skip_whitespace()

                return word

            start_index = end_index
            end_index = start_index + 1

            if start_index == len(self.text):
                return None

    def peek_char(self, offset: int = 0) -> str:
        """
        Returns the character at the current index plus the specified offset without advancing the index.

        Args:
            offset (int, optional): The number of positions ahead of the current index to peek. Defaults to 0.

        Returns:
            str: The character at the specified position if within bounds; otherwise, None.
        """
        if (self.index + offset) < len(self.text):
            return self.text[self.index + offset]
        return None

    def skip_char(self, count: int = 1) -> None:
        """
        Skips a specified number of characters in the text by advancing the current index.
        Args:
            count (int, optional): The number of characters to skip. Defaults to 1.
        Notes:
            If the requested count exceeds the remaining characters, the index is set to the end of the text.
        """
        if self.index + count > len(self.text):
            count = len(self.text) - self.index

        self.index += count

    def skip(self, count: int) -> None:
        """
        Skips a specified number of characters in the text, updating the current index and line number.
        If the skip would exceed the length of the text, it only skips up to the end.
        Increments the current line counter for each newline character encountered.
        After skipping, advances past any subsequent whitespace characters.
        Args:
            count (int): The number of characters to skip.
        """
        if self.index + count > len(self.text):
            count = len(self.text) - self.index

        for i in range(count):
            if self.text[self.index + i] == "\n":
                self.current_line += 1

        self.index += count
        self.skip_whitespace()

    def skip_line(self) -> None:
        """
        Skips the current line in the text by advancing the index to the next newline character,
        then skips any subsequent whitespace characters.
        """
        while self.index != len(self.text) and self.text[self.index] != "\n":
            self.index += 1

        self.skip_whitespace()

    def find_next_of(self, chars: set[str]) -> str:
        """
        Finds and returns the next character in the text, starting from the current index, that matches any character in the given set.
        Args:
            chars (set[str]): A set of characters to search for.
        Returns:
            str: The next character in the text that is present in the 'chars' set.
        Raises:
            IndexError: If no matching character is found before the end of the text.
        """
        offset = 0

        while self.text[self.index + offset] not in chars:
            offset += 1

        return self.text[self.index + offset]
