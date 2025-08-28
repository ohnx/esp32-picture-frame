import os
import random
import argparse
from flask import Flask, send_from_directory, jsonify, request, abort, Response
from PIL import Image
import numpy as np

app = Flask(__name__, static_folder="static")
PHOTO_DIR = None  # will be set in main()


palette = [
    (0, 0, 0),
    (255, 255, 255),
    (0, 255, 0),
    (0, 0, 255),
    (255, 0, 0),
    (255, 255, 0),
    (255, 128, 0),
]
# Make a dict for quick lookup
color_to_index = {c: i for i, c in enumerate(palette)}

def image_to_packed_bytes(path):
    img = Image.open(path).convert("RGB")
    pixels = np.array(img).reshape(-1, 3)

    # Map each pixel directly to its palette index
    indices = [color_to_index[tuple(px)] for px in pixels]

    # Pack 2 pixels per byte
    packed = bytearray()
    for i in range(0, len(indices), 2):
        hi = indices[i] & 0x0F
        lo = indices[i+1] & 0x0F if i+1 < len(indices) else 0
        packed.append((hi << 4) | lo)

    return bytes(packed)

@app.route("/")
def index():
    """Serve index.html (frontend)."""
    return send_from_directory(app.static_folder, "index.html")


@app.route("/list")
def list_files():
    """Return JSON list of all photo filenames."""
    files = []
    for f in os.listdir(PHOTO_DIR):
        path = os.path.join(PHOTO_DIR, f)
        if os.path.isfile(path):
            files.append(f)
    return jsonify(files)


@app.route("/photo")
def random_photo():
    """Return a random photo from the directory."""
    files = [f for f in os.listdir(PHOTO_DIR) if os.path.isfile(os.path.join(PHOTO_DIR, f))]
    if not files:
        abort(404, "No photos found")
    chosen = random.choice(files)
    fmt = request.args.get('format', default='png', type=str)
    if fmt == 'png':
        return send_from_directory(PHOTO_DIR, chosen)
    elif fmt == 'packed':
        try:
            return Response(image_to_packed_bytes(PHOTO_DIR + '/' + chosen), mimetype="application/octet-stream")
        except KeyError:
            print("Should delete image {} as it's invalid".format(chosen))
            abort(404, "Bad image {}".format(chosen))
    else:
        abort(404, "Don't know how to generate format {}".format(fmt))

@app.route("/photo/<filename>")
def get_photo(filename):
    """Return a specific photo by filename."""
    filepath = os.path.join(PHOTO_DIR, filename)
    if not os.path.exists(filepath):
        abort(404, "File not found")
    return send_from_directory(PHOTO_DIR, filename)


@app.route("/upload", methods=["POST"])
def upload_file():
    """Upload a new photo to the library."""
    if "file" not in request.files:
        abort(400, "No file part")
    file = request.files["file"]
    if file.filename == "":
        abort(400, "No selected file")

    filepath = os.path.join(PHOTO_DIR, file.filename)
    file.save(filepath)
    return jsonify({"status": "success", "filename": file.filename})


@app.route("/delete/<filename>", methods=["DELETE"])
def delete_file(filename):
    """Delete a photo by filename."""
    filepath = os.path.join(PHOTO_DIR, filename)
    if os.path.exists(filepath):
        os.remove(filepath)
        return jsonify({"status": "deleted", "filename": filename})
    else:
        abort(404, "File not found")


def main():
    global PHOTO_DIR

    parser = argparse.ArgumentParser(description="Photo library Flask app")
    parser.add_argument("--path", type=str, default="photos", help="Path to photo directory")
    parser.add_argument("--host", type=str, default="0.0.0.0", help="Host to bind")
    parser.add_argument("--port", type=int, default=8901, help="Port to run the server on")

    args = parser.parse_args()

    PHOTO_DIR = args.path
    os.makedirs(PHOTO_DIR, exist_ok=True)

    app.run(host=args.host, port=args.port, debug=True)


if __name__ == "__main__":
    main()
