#!/usr/bin/env python3
"""
quine_generator.py
Creates a self-replicating version of ft_shield.c
Usage: python3 quine_generator.py <input_file> <output_file>
"""

import sys
import os
import re

def escape_for_c_string(text):
    """
    Escape text to be safely embedded in a C string literal.
    """
    # Must escape in this order: backslash first, then quotes
    escaped = text.replace('\\', '\\\\')   # Escape backslashes
    escaped = escaped.replace('"', '\\"')  # Escape double quotes
    escaped = escaped.replace('\n', '\\n"\n"')  # Split long string across lines
    
    # Remove the trailing "\n" on the last line
    if escaped.endswith('\n"'):
        escaped = escaped[:-3] + '"'
    
    return escaped

def extract_quine_function(source):
    """
    Find and extract the quine() function from the source code.
    Returns: (before_quine, quine_function, after_quine)
    """
    # Pattern to find the quine function
    # We'll look for "void	quine(void)" and capture until the matching brace
    pattern = r'(void\s+quine\s*\(\s*void\s*\)\s*\{[^}]*?(?:\{[^}]*?\}[^}]*?)*\})'
    
    match = re.search(pattern, source, re.DOTALL)
    
    if not match:
        # If pattern doesn't match, try a simpler approach
        # Look for the function signature and capture everything until "}"
        start = source.find("void\tquine(void)")
        if start == -1:
            start = source.find("void quine(void)")
        
        if start == -1:
            raise ValueError("Could not find quine function in source code")
        
        # Find the matching brace
        brace_count = 0
        i = source.find('{', start)
        if i == -1:
            raise ValueError("Could not find opening brace for quine function")
        
        brace_start = i
        brace_count = 1
        i += 1
        
        while i < len(source) and brace_count > 0:
            if source[i] == '{':
                brace_count += 1
            elif source[i] == '}':
                brace_count -= 1
            i += 1
        
        if brace_count > 0:
            raise ValueError("Unbalanced braces in quine function")
        
        quine_end = i
        quine_function = source[start:quine_end]
        before_quine = source[:start]
        after_quine = source[quine_end:]
        
        return before_quine, quine_function, after_quine
    
    # Extract using regex
    quine_function = match.group(1)
    start = match.start()
    end = match.end()
    
    before_quine = source[:start]
    after_quine = source[end:]
    
    return before_quine, quine_function, after_quine

def create_quine_version(input_file, output_file):
    """
    Create a self-replicating version of the C program.
    """
    # Read the original source
    with open(input_file, 'r') as f:
        original_source = f.read()
    
    # Extract the quine function and split the source
    try:
        before_quine, original_quine, after_quine = extract_quine_function(original_source)
    except ValueError as e:
        print(f"Error: {e}")
        print("Attempting manual extraction...")
        
        # Manual extraction as fallback
        lines = original_source.split('\n')
        quine_start = -1
        quine_end = -1
        brace_count = 0
        
        for i, line in enumerate(lines):
            if "void" in line and "quine" in line and "void" in line:
                quine_start = i
                # Count opening braces in this line
                brace_count += line.count('{')
                brace_count -= line.count('}')
                break
        
        if quine_start == -1:
            print("Could not find quine function. Creating a new one.")
            # Create a default split (assume quine is at the end)
            before_quine = original_source
            original_quine = ""
            after_quine = ""
        else:
            # Find the end of the function
            for i in range(quine_start, len(lines)):
                brace_count += lines[i].count('{')
                brace_count -= lines[i].count('}')
                
                if brace_count == 0 and i > quine_start:
                    quine_end = i + 1
                    break
            
            if quine_end == -1:
                quine_end = len(lines)
            
            before_quine = '\n'.join(lines[:quine_start])
            original_quine = '\n'.join(lines[quine_start:quine_end])
            after_quine = '\n'.join(lines[quine_end:])
    
    # Create the new quine function with embedded source
    # First, create the template for the new source (with placeholder)
    new_quine_template = '''void	quine(void)
{
	char *source_code = "@@EMBEDDED_SOURCE@@";
	int fd_dst = open(TEMP_SHIELD, O_WRONLY | O_CREAT | O_TRUNC, 0755);
	if (fd_dst == -1) {
		ft_perror(TEMP_SHIELD);
		return ;
	}
	write(fd_dst, source_code, strlen(source_code));
	close(fd_dst);
	system("gcc "TEMP_SHIELD" -o "DEST_PATH);
}
'''
    
    # Create the new source with the template
    new_source_with_placeholder = before_quine + new_quine_template + after_quine
    
    # Escape the new source for embedding
    escaped_source = escape_for_c_string(new_source_with_placeholder)
    
    # Replace the placeholder with the actual escaped source
    final_quine = new_quine_template.replace('@@EMBEDDED_SOURCE@@', escaped_source)
    
    # Create the final source code
    final_source = before_quine + final_quine + after_quine
    
    # Write the new quine version
    with open(output_file, 'w') as f:
        f.write(final_source)
    
    print(f"Generated quine version: {output_file}")
    print(f"Original size: {len(original_source)} characters")
    print(f"Quine size: {len(final_source)} characters")


def main():
    if len(sys.argv) != 3:
        print("Usage: python3 quine_generator.py <input_file> <output_file>")
        print("Example: python3 quine_generator.py ft_shield.c ft_shield_quine.c")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    if not os.path.exists(input_file):
        print(f"Error: Input file '{input_file}' not found")
        sys.exit(1)
    
    create_quine_version(input_file, output_file)

if __name__ == "__main__":
    main()