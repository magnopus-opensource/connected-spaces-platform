terraform {
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "5.84.0"
    }
    null = {
      source  = "hashicorp/null"
      version = "3.2.3"
    }
  }

  backend "s3" {
    // Leave blank. This will be configured by the invoker.
  }
}

provider "aws" {
  region = "us-east-1"

  default_tags {
    tags = {
      Project     = var.project
      Environment = "${var.environment}-Virginia"
    }
  }
}
