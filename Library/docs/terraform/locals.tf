locals {
  mime_types = jsondecode(file("${path.module}/mime_types.json"))
}
