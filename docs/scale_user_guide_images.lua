-- Keep portrait screenshots compact and leave room for their captions.
function Image(image)
    if not image.attributes.width and not image.attributes.height then
        image.attributes.width = "40%"
    end
    return image
end
