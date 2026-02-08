#!/usr/bin/env python3
"""
Convert image file (PNG/JPEG/GIF) to C array for Arduino
Usage: python image_to_c_array.py input_image.png output_array_name
"""

import sys
import os

def image_to_c_array(input_file, array_name="image_data", output_file=None):
    """Convert image file to C array and save to .h file"""
    
    if not os.path.exists(input_file):
        print(f"Error: File '{input_file}' not found")
        sys.exit(1)
    
    # Read binary data
    with open(input_file, 'rb') as f:
        data = f.read()
    
    # Detect MIME type from header
    mime_type = "image/png"  # default
    if data[:8] == b'\x89PNG\r\n\x1a\n':
        mime_type = "image/png"
    elif data[:2] == b'\xff\xd8':
        mime_type = "image/jpeg"
    elif data[:6] in (b'GIF87a', b'GIF89a'):
        mime_type = "image/gif"
    
    file_size = len(data)
    
    # Generate output filename if not provided
    if output_file is None:
        output_file = f"{array_name}.h"
    
    # Write to file
    with open(output_file, 'w') as out:
        out.write(f"// Generated from: {input_file}\n")
        out.write(f"// Size: {file_size} bytes\n")
        out.write(f"// MIME type: {mime_type}\n")
        out.write(f"const uint8_t {array_name}[] PROGMEM = {{\n")
        
        # Output in rows of 16 bytes
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            hex_values = ', '.join(f'0x{b:02X}' for b in chunk)
            if i + 16 < len(data):
                out.write(f"  {hex_values},\n")
            else:
                out.write(f"  {hex_values}\n")
        
        out.write("};\n")
        out.write("\n")
        out.write(f"// Usage example:\n")
        out.write(f"// server.setLogoFromImage({array_name}, sizeof({array_name}), \"{mime_type}\");\n")
    
    print(f"✓ Created: {output_file}")
    print(f"  Size: {file_size} bytes")
    print(f"  MIME: {mime_type}")
    print(f"  Array: {array_name}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python image_to_c_array.py input_image.png [array_name] [output_file.h]")
        print("Example: python image_to_c_array.py logo.png custom_logo")
        print("         python image_to_c_array.py logo.png custom_logo logo.h")
        sys.exit(1)
    
    input_file = sys.argv[1]
    array_name = sys.argv[2] if len(sys.argv) > 2 else "image_data"
    output_file = sys.argv[3] if len(sys.argv) > 3 else None
    
    image_to_c_array(input_file, array_name, output_file)
