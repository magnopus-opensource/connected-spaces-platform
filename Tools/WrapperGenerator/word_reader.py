from typing import List

class WordReader:
    """ A helper class for reading a string of text word-by-word """


    text: str
    index: int
    current_line: int = 1

    WHITESPACE_CHARS: List[str] = [' ', '\t', '\r', '\n']


    def __init__(self, text: str):
        self.text = text
        self.index = 0

    
    def skip_whitespace(self) -> None:
        while self.index != len(self.text) and self.text[self.index] in self.WHITESPACE_CHARS:
            if self.text[self.index] == '\n':
                self.current_line += 1
            
            self.index += 1
    

    def next_word(self, delimiters: List[str] = [' ', '\t', '\n', ':', ';', '{', '}', ',', '<', '>', '(', ')', '=', '#', '~', '*', '&', '[', ']'], return_empty: bool = False) -> str:
        if self.index == len(self.text):
            return None
        
        start_index = self.index
        # Always consume the first character
        end_index = self.index + 1

        while True:
            next_char = self.peek_char()

            if next_char == '\n':
                self.current_line += 1

            if next_char in delimiters and next_char not in self.WHITESPACE_CHARS:
                self.index = end_index
                self.skip_whitespace()

                return self.text[start_index]

            while end_index != len(self.text) and self.text[end_index] not in delimiters:
                end_index += 1
            
            word = self.text[start_index:end_index].strip()

            if return_empty or word != '':
                self.index = end_index
                self.skip_whitespace()

                return word
            
            start_index = end_index
            end_index = start_index + 1

            if start_index == len(self.text):
                return None
    

    def peek_char(self, offset: int = 0) -> str:
        return None if (self.index + offset) >= len(self.text) else self.text[self.index + offset]
    

    def skip_char(self, count: int = 1) -> None:
        if self.index + count > len(self.text):
            count = len(self.text) - self.index
        
        self.index += count


    def skip(self, count: int) -> None:
        if self.index + count > len(self.text):
            count = len(self.text) - self.index
        
        for i in range(count):
            if self.text[self.index + i] == '\n':
                self.current_line += 1

        self.index += count
        self.skip_whitespace()


    def skip_line(self) -> None:
        while self.index != len(self.text) and self.text[self.index] != '\n':
            self.index += 1
        
        self.skip_whitespace()
    

    def find_next_of(self, chars: List[str]) -> str:
        offset = 0

        while self.text[self.index + offset] not in chars:
            offset += 1
        
        return self.text[self.index + offset]
